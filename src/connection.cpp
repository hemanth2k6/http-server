
#include "connection.hpp"
#include "proxy_server.hpp"
#include "epoll_manager.hpp"
#include "rate_limiter.hpp"
#include "load_balancer.hpp"
#include "socket_utils.hpp"
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static const size_t MAX_REQUEST_SIZE = 1 * 1024 * 1024;

ClientConnection::ClientConnection(int fd, EpollManager &epoll, ProxyServer &server)
    : fd_(fd), epoll_(epoll), server_(server), state_(State::READING_REQUEST),
      write_offset_(0), response_ready_(false)
{
    epoll_.add_fd(fd_, EPOLLIN | EPOLLET, this);
}

ClientConnection::~ClientConnection()
{
    if (fd_ >= 0)
    {
        epoll_.del_fd(fd_);
        close(fd_);
        fd_ = -1;
    }
}

void ClientConnection::handle_event(uint32_t events)
{
    if (events & (EPOLLERR | EPOLLHUP))
    {
        initiate_close();
        return;
    }
    if (events & EPOLLIN)
    {
        process_read();
    }
    if (events & EPOLLOUT)
    {
        process_write();
    }
}

void ClientConnection::process_read()
{
    if (state_ != State::READING_REQUEST)
        return;
    char buf[4096];
    while (true)
    {
        ssize_t n = recv(fd_, buf, sizeof(buf), 0);
        if (n > 0)
        {
            read_buffer_.append(buf, n);
            if (read_buffer_.size() > MAX_REQUEST_SIZE)
            {
                send_error_response(413, "Request Entity Too Large");
                return;
            }
            HttpRequestParser::ParseResult res = parser_.parse(read_buffer_);
            if (res == HttpRequestParser::ParseResult::COMPLETE)
            {
                sockaddr_in addr;
                socklen_t len = sizeof(addr);
                getpeername(fd_, reinterpret_cast<sockaddr *>(&addr), &len);
                std::string client_ip = inet_ntoa(addr.sin_addr);
                if (!server_.rate_limiter().allow(client_ip))
                {
                    send_error_response(429, "Too Many Requests");
                    return;
                }
                start_backend_request();
                return;
            }
            else if (res == HttpRequestParser::ParseResult::ERROR)
            {
                send_error_response(400, "Bad Request");
                return;
            }
        }
        else if (n == 0)
        {
            initiate_close();
            return;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else
            {
                initiate_close();
                return;
            }
        }
    }
}

void ClientConnection::start_backend_request()
{
    auto backend_info = server_.load_balancer().get_backend();
    if (!backend_info)
    {
        send_error_response(503, "Service Unavailable");
        return;
    }
    int backend_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (backend_fd < 0)
    {
        send_error_response(503, "Service Unavailable");
        return;
    }
    socket_utils::set_nonblocking(backend_fd);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(backend_info->port);
    inet_pton(AF_INET, backend_info->host.c_str(), &addr.sin_addr);
    int ret = connect(backend_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    if (ret < 0 && errno != EINPROGRESS)
    {
        close(backend_fd);
        send_error_response(502, "Bad Gateway");
        return;
    }
    std::string request_to_send = parser_.reconstruct_request();
    request_to_send += "Connection: close\r\n\r\n";
    size_t body_start = parser_.get_header_end();
    size_t body_len = parser_.get_content_length();
    if (body_len > 0 && read_buffer_.size() >= body_start + body_len)
    {
        request_to_send += read_buffer_.substr(body_start, body_len);
    }
    backend_ = std::make_unique<BackendConnection>(backend_fd, this, epoll_, request_to_send);
    epoll_.add_fd(backend_fd, EPOLLOUT | EPOLLET, backend_.get());
    state_ = State::WAITING_BACKEND;
    uint32_t events = EPOLLIN | EPOLLET;
    epoll_.mod_fd(fd_, events, this);
}

void ClientConnection::on_backend_response(std::string response)
{
    write_buffer_ = std::move(response);
    write_offset_ = 0;
    response_ready_ = true;
    state_ = State::SENDING_RESPONSE;
    uint32_t events = EPOLLOUT | EPOLLET;
    epoll_.mod_fd(fd_, events, this);
    backend_.reset();
}

void ClientConnection::process_write()
{
    if (state_ != State::SENDING_RESPONSE || !response_ready_)
        return;
    while (write_offset_ < write_buffer_.size())
    {
        ssize_t n = send(fd_, write_buffer_.data() + write_offset_, write_buffer_.size() - write_offset_, 0);
        if (n > 0)
        {
            write_offset_ += n;
        }
        else if (n == 0)
        {
            break;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else
            {
                initiate_close();
                return;
            }
        }
    }
    if (write_offset_ == write_buffer_.size())
    {
        initiate_close();
    }
}

void ClientConnection::send_error_response(int status_code, const std::string &message)
{
    std::string body = "<html><body><h1>" + std::to_string(status_code) + " " + message + "</h1></body></html>";
    std::string response = "HTTP/1.1 " + std::to_string(status_code) + " " + message + "\r\nContent-Length: " + std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
    write_buffer_ = response;
    write_offset_ = 0;
    response_ready_ = true;
    state_ = State::SENDING_RESPONSE;
    uint32_t events = EPOLLOUT | EPOLLET;
    epoll_.mod_fd(fd_, events, this);
}

void ClientConnection::initiate_close()
{
    if (state_ == State::CLOSING)
        return;
    state_ = State::CLOSING;
    if (backend_)
    {
        epoll_.del_fd(backend_->get_fd());
        backend_.reset();
    }
    epoll_.del_fd(fd_);
    close(fd_);
    fd_ = -1;
}

BackendConnection::BackendConnection(int fd, ClientConnection *client, EpollManager &epoll, const std::string &request_data)
    : fd_(fd), epoll_(epoll), client_(client), state_(State::CONNECTING),
      request_data_(request_data), send_offset_(0) {}

BackendConnection::~BackendConnection()
{
    if (fd_ >= 0)
    {
        epoll_.del_fd(fd_);
        close(fd_);
        fd_ = -1;
    }
}

void BackendConnection::handle_event(uint32_t events)
{
    if (events & (EPOLLERR | EPOLLHUP))
    {
        if (client_)
            client_->on_backend_response("");
        return;
    }
    if (state_ == State::CONNECTING && (events & EPOLLOUT))
    {
        int err = 0;
        socklen_t len = sizeof(err);
        getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &len);
        if (err != 0)
        {
            if (client_)
                client_->on_backend_response("");
            return;
        }
        state_ = State::SENDING_REQUEST;
        start_sending();
    }
    else if (state_ == State::SENDING_REQUEST && (events & EPOLLOUT))
    {
        start_sending();
    }
    else if (state_ == State::READING_RESPONSE && (events & EPOLLIN))
    {
        char buf[4096];
        while (true)
        {
            ssize_t n = recv(fd_, buf, sizeof(buf), 0);
            if (n > 0)
            {
                response_buffer_.append(buf, n);
            }
            else if (n == 0)
            {
                state_ = State::DONE;
                if (client_)
                    client_->on_backend_response(std::move(response_buffer_));
                return;
            }
            else
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                else
                {
                    if (client_)
                        client_->on_backend_response("");
                    return;
                }
            }
        }
    }
}

void BackendConnection::start_sending()
{
    while (send_offset_ < request_data_.size())
    {
        ssize_t n = send(fd_, request_data_.data() + send_offset_, request_data_.size() - send_offset_, 0);
        if (n > 0)
        {
            send_offset_ += n;
        }
        else if (n == 0)
        {
            break;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else
            {
                if (client_)
                    client_->on_backend_response("");
                return;
            }
        }
    }
    if (send_offset_ == request_data_.size())
    {
        state_ = State::READING_RESPONSE;
        uint32_t events = EPOLLIN | EPOLLET;
        epoll_.mod_fd(fd_, events, this);
    }
}