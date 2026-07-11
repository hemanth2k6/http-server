#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "proxy_server.hpp"
#include "connection.hpp"
#include "socket_utils.hpp"
#include <unistd.h>
#include <csignal>
#include <chrono>
#include <netinet/in.h>
#include <sys/socket.h>

static volatile sig_atomic_t shutdown_flag = 0;

static void signal_handler(int)
{
    shutdown_flag = 1;
}

ProxyServer::ProxyServer(int listen_port, const std::vector<BackendInfo> &backends, double rate, double burst, int health_check_interval)
    : listen_fd_(-1),
      rate_limiter_(rate, burst),
      load_balancer_(backends),
      health_checker_(const_cast<std::vector<BackendInfo> &>(backends), health_check_interval),
      running_(false)
{
    listen_fd_ = socket_utils::create_listen_socket(listen_port);
    socket_utils::set_nonblocking(listen_fd_);
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGPIPE, SIG_IGN);
}

ProxyServer::~ProxyServer()
{
    if (listen_fd_ >= 0)
    {
        close(listen_fd_);
    }
}

void ProxyServer::run()
{
    epoll_.add_fd(listen_fd_, EPOLLIN, nullptr);
    running_ = true;
    bool stopping = false;
    std::chrono::steady_clock::time_point deadline;
    std::vector<epoll_event> events;

    while (running_)
    {
        if (shutdown_flag && !stopping)
        {
            stopping = true;
            epoll_.del_fd(listen_fd_);
            close(listen_fd_);
            listen_fd_ = -1;
            deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        }
        int timeout = 1000;
        if (stopping)
        {
            auto now = std::chrono::steady_clock::now();
            if (now >= deadline)
                break;
            timeout = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();
            if (timeout < 0)
                timeout = 0;
        }
        int nfds = epoll_.wait(events, timeout);
        for (int i = 0; i < nfds; ++i)
        {
            if (events[i].data.ptr == nullptr)
            {
                if (!stopping)
                {
                    while (true)
                    {
                        sockaddr_in client_addr;
                        socklen_t len = sizeof(client_addr);
                        int client_fd = accept4(listen_fd_, reinterpret_cast<sockaddr *>(&client_addr), &len, SOCK_NONBLOCK);
                        if (client_fd < 0)
                            break;
                        auto conn = std::make_unique<ClientConnection>(client_fd, epoll_, *this);
                        clients_.emplace(client_fd, std::move(conn));
                    }
                }
            }
            else
            {
                ConnectionBase *base = static_cast<ConnectionBase *>(events[i].data.ptr);
                if (base->is_backend())
                {
                    static_cast<BackendConnection *>(base)->handle_event(events[i].events);
                }
                else
                {
                    static_cast<ClientConnection *>(base)->handle_event(events[i].events);
                }
            }
        }
        for (auto it = clients_.begin(); it != clients_.end();)
        {
            if (it->second->get_fd() == -1)
            {
                it = clients_.erase(it);
            }
            else
            {
                ++it;
            }
        }
        if (stopping && clients_.empty())
            break;
    }
    clients_.clear();
}