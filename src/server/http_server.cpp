#include "server/http_server.hpp"
#include "utils/socket_utils.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>

http_server::http_server(int port, int thread_count, const std::string &doc_root)
    : port_(port), thread_count_(thread_count), doc_root_(doc_root), server_fd_(-1), running_(false), pool_(std::make_unique<thread_pool>(thread_count)), file_handler_(doc_root)
{

    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0)
    {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        close(server_fd_);
        throw std::runtime_error("Failed to set socket options");
    }

    address_.sin_family = AF_INET;
    address_.sin_addr.s_addr = INADDR_ANY;
    address_.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr *)&address_, sizeof(address_)) < 0)
    {
        close(server_fd_);
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(server_fd_, SOMAXCONN) < 0)
    {
        close(server_fd_);
        throw std::runtime_error("Failed to listen on socket");
    }
}

http_server::~http_server()
{
    stop();
    if (server_fd_ >= 0)
    {
        close(server_fd_);
    }
}

void http_server::start()
{
    if (running_)
        return;
    running_ = true;
    accept_thread_ = std::make_unique<std::thread>(&http_server::accept_connections, this);
    std::cout << "Server started on port " << port_ << " with " << thread_count_ << " threads" << std::endl;
}

void http_server::stop()
{
    if (!running_)
        return;
    running_ = false;
    if (accept_thread_ && accept_thread_->joinable())
    {
        accept_thread_->join();
    }
    pool_->shutdown();
    std::cout << "Server stopped" << std::endl;
}

void http_server::accept_connections()
{
    while (running_)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd_, (struct sockaddr *)&client_addr, &client_len);

        if (client_fd < 0)
        {
            if (running_)
            {
                std::cerr << "Accept failed" << std::endl;
            }
            continue;
        }

        pool_->enqueue([this, client_fd]()
                       { this->handle_client(client_fd); });
    }
}

void http_server::handle_client(int client_fd)
{
    char buffer[65536];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0)
    {
        close(client_fd);
        return;
    }

    buffer[bytes_read] = '\0';

    auto request = parser_.parse(buffer);
    if (!request.is_valid)
    {
        auto response = builder_.build_error(400, "Bad Request");
        send(client_fd, response.data(), response.size(), 0);
        close(client_fd);
        return;
    }

    if (request.method == "GET")
    {
        std::string path = request.uri;
        if (path == "/")
            path = "/index.html";

        std::string file_path = doc_root_ + path;
        std::string content;

        if (file_handler_.read_file(file_path, content))
        {
            std::string mime = file_handler_.get_mime_type(file_path);
            auto response = builder_.build_ok(content, mime);
            send(client_fd, response.data(), response.size(), 0);
        }
        else
        {
            auto response = builder_.build_error(404, "Not Found");
            send(client_fd, response.data(), response.size(), 0);
        }
    }
    else if (request.method == "HEAD")
    {
        std::string path = request.uri;
        if (path == "/")
            path = "/index.html";

        std::string file_path = doc_root_ + path;
        std::string content;

        if (file_handler_.read_file(file_path, content))
        {
            std::string mime = file_handler_.get_mime_type(file_path);
            auto response = builder_.build_head_ok(content.size(), mime);
            send(client_fd, response.data(), response.size(), 0);
        }
        else
        {
            auto response = builder_.build_error(404, "Not Found");
            send(client_fd, response.data(), response.size(), 0);
        }
    }
    else
    {
        auto response = builder_.build_error(405, "Method Not Allowed");
        send(client_fd, response.data(), response.size(), 0);
    }

    close(client_fd);
}