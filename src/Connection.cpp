#include "Connection.hpp"
#include "EventLoop.hpp"
#include "LoadBalancer.hpp"
#include "RateLimiter.hpp"
#include "UpstreamPool.hpp"
#include "HttpParser.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>

Connection::Connection(int fd, EventLoop *loop, Type type)
    : fd_(fd), loop_(loop), type_(type),
      state_(type == Type::CLIENT ? State::READING_REQUEST : State::CONNECTING),
      backend_(nullptr) {}

Connection::~Connection() { close_connection(); }

void Connection::set_upstream(std::shared_ptr<Connection> upstream) { upstream_ = upstream; }
void Connection::set_backend(Backend *backend) { backend_ = backend; }
void Connection::set_client_ip(const std::string &ip) { client_ip_ = ip; }
void Connection::set_request(std::string &&req) { request_ = std::move(req); }
Backend *Connection::get_backend() const { return backend_; }
bool Connection::is_alive() const { return state_ != State::CLOSING && fd_ != -1; }

void Connection::reset_state()
{
    read_buffer_.clear();
    write_buffer_.clear();
    write_offset_ = 0;
    request_.clear();
    state_ = State::READING_REQUEST; // upstream will change to READY after connect
    connected_ = true;
}

void Connection::handle_events(uint32_t events)
{
    if (events & (EPOLLERR | EPOLLHUP))
    {
        handle_error();
        return;
    }
    if (events & EPOLLIN)
        handle_read();
    if (events & EPOLLOUT)
        handle_write();
}

void Connection::handle_read()
{
    char buf[16384];
    ssize_t n;
    while ((n = recv(fd_, buf, sizeof(buf), 0)) > 0)
    {
        read_buffer_.insert(read_buffer_.end(), buf, buf + n);
    }

    if (n == 0)
    { // EOF
        if (type_ == Type::CLIENT)
        {
            // Client closed connection – we can close upstream too
            schedule_close();
        }
        else
        {
            // Upstream closed – forward remaining data to client if any
            if (auto client = upstream_.lock())
            {
                if (!read_buffer_.empty())
                {
                    std::string resp(read_buffer_.begin(), read_buffer_.end());
                    client->write_response(resp);
                    read_buffer_.clear();
                }
            }
            schedule_close();
        }
        return;
    }

    if (n == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        handle_error();
        return;
    }

    // ---------- Client side: reading request ----------
    if (type_ == Type::CLIENT && state_ == State::READING_REQUEST)
    {
        auto req = HttpParser::parse(read_buffer_);
        if (!req)
            return; // need more data
        if (!req->complete)
        {
            loop_->update_events(fd_, EPOLLIN | EPOLLET);
            return;
        }

        // Rate limit
        if (!loop_->get_rate_limiter().allow(client_ip_))
        {
            send_http_error(429, "Too Many Requests");
            schedule_close();
            return;
        }

        // Pick backend
        Backend *backend = loop_->get_load_balancer().next();
        if (!backend)
        {
            send_http_error(503, "Service Unavailable");
            schedule_close();
            return;
        }

        // Get or create upstream connection
        auto upstream = loop_->get_upstream_pool().get_or_create(backend);
        if (!upstream)
        {
            loop_->get_load_balancer().mark_failure(backend);
            send_http_error(503, "Service Unavailable");
            schedule_close();
            return;
        }

        // Link both ends
        upstream->set_upstream(shared_from_this());
        this->set_upstream(upstream);

        // Forward the request
        std::string req_str(read_buffer_.begin(), read_buffer_.end());
        upstream->write_response(req_str);
        read_buffer_.clear();
        state_ = State::FORWARDING_REQUEST;
        loop_->update_events(upstream->fd(), EPOLLOUT | EPOLLIN | EPOLLET);
        return;
    }

    // ---------- Upstream side: reading response ----------
    if (type_ == Type::UPSTREAM && state_ == State::READING_RESPONSE)
    {
        if (auto client = upstream_.lock())
        {
            std::string resp(read_buffer_.begin(), read_buffer_.end());
            client->write_response(resp);
            read_buffer_.clear();
            // We will wait for more data (chunked or keep-alive) – but for simplicity,
            // we close after response. We can improve later.
            // For now, we close after first response (no keep-alive).
            schedule_close(); // will recycle upstream if possible
        }
        else
        {
            schedule_close();
        }
    }
}

void Connection::handle_write()
{
    // Special case: upstream still connecting
    if (type_ == Type::UPSTREAM && state_ == State::CONNECTING)
    {
        int err = 0;
        socklen_t len = sizeof(err);
        if (getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &len) == -1 || err != 0)
        {
            handle_error();
            return;
        }
        connected_ = true;
        state_ = State::READING_RESPONSE; // ready to read response
        loop_->update_events(fd_, EPOLLIN | EPOLLET);
        // If we have a pending request, send it now
        if (!request_.empty())
        {
            write_response(request_);
            request_.clear();
        }
        return;
    }

    // Normal write
    if (write_buffer_.empty())
    {
        loop_->update_events(fd_, EPOLLIN | EPOLLET);
        return;
    }

    ssize_t n = send(fd_, write_buffer_.data() + write_offset_,
                     write_buffer_.size() - write_offset_, MSG_NOSIGNAL);
    if (n <= 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            loop_->update_events(fd_, EPOLLOUT | EPOLLIN | EPOLLET);
            return;
        }
        handle_error();
        return;
    }

    write_offset_ += n;
    if (write_offset_ >= write_buffer_.size())
    {
        write_buffer_.clear();
        write_offset_ = 0;
        // If we are a client and just sent the request, we now expect response
        if (type_ == Type::CLIENT && state_ == State::FORWARDING_REQUEST)
        {
            state_ = State::READING_RESPONSE; // Wait for upstream to write back
            loop_->update_events(fd_, EPOLLIN | EPOLLET);
            return;
        }
        // If we are upstream and just sent the request, we wait for response (already set)
        if (type_ == Type::UPSTREAM)
        {
            // We already are in READING_RESPONSE
            loop_->update_events(fd_, EPOLLIN | EPOLLET);
            return;
        }
        // If we are client and just finished sending response to client, close or keep-alive
        if (type_ == Type::CLIENT && state_ == State::READING_RESPONSE)
        {
            // We have finished sending the response to the client.
            // For simplicity, we close.
            schedule_close();
            return;
        }
    }
    else
    {
        loop_->update_events(fd_, EPOLLOUT | EPOLLIN | EPOLLET);
    }
}

void Connection::write_response(const std::string &response)
{
    write_buffer_.insert(write_buffer_.end(), response.begin(), response.end());
    loop_->update_events(fd_, EPOLLOUT | EPOLLIN | EPOLLET);
}

void Connection::handle_error()
{
    if (type_ == Type::UPSTREAM && backend_)
    {
        loop_->get_load_balancer().mark_failure(backend_);
    }
    schedule_close();
}

void Connection::schedule_close()
{
    if (state_ == State::CLOSING)
        return;
    state_ = State::CLOSING;
    if (type_ == Type::UPSTREAM && backend_)
    {
        // Recycle the connection if it's still usable
        loop_->get_upstream_pool().recycle(shared_from_this());
    }
    else
    {
        close_connection();
    }
}

void Connection::close_connection()
{
    if (fd_ != -1)
    {
        loop_->remove_event(fd_);
        close(fd_);
        fd_ = -1;
    }
}

void Connection::send_http_error(int code, const std::string &msg)
{
    std::string resp = "HTTP/1.1 " + std::to_string(code) + " " + msg + "\r\n"
                                                                        "Content-Length: 0\r\n"
                                                                        "Connection: close\r\n\r\n";
    write_response(resp);
}