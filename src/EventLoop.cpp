#include "EventLoop.hpp"
#include "Connection.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <signal.h>

EventLoop::EventLoop(const Config &cfg)
    : cfg_(cfg), lb_(cfg.backends), limiter_(cfg.rate_limit), pool_(this)
{
    epfd_ = epoll_create1(0);
    if (epfd_ == -1)
    {
        perror("epoll_create1");
        exit(1);
    }
    signal(SIGPIPE, SIG_IGN);
}

EventLoop::~EventLoop()
{
    close(epfd_);
    if (listen_fd_ != -1)
        close(listen_fd_);
}

void EventLoop::setup_listener()
{
    listen_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listen_fd_ == -1)
    {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(cfg_.port);

    if (bind(listen_fd_, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        exit(1);
    }
    if (listen(listen_fd_, 1024) == -1)
    {
        perror("listen");
        exit(1);
    }

    struct epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listen_fd_;
    if (epoll_ctl(epfd_, EPOLL_CTL_ADD, listen_fd_, &ev) == -1)
    {
        perror("epoll_ctl add listener");
        exit(1);
    }
}

void EventLoop::run()
{
    setup_listener();
    const int MAX_EVENTS = 4096;
    struct epoll_event events[MAX_EVENTS];

    while (running_)
    {
        int nfds = epoll_wait(epfd_, events, MAX_EVENTS, -1);
        if (nfds == -1)
        {
            if (errno == EINTR)
                continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; ++i)
        {
            int fd = events[i].data.fd;
            if (fd == listen_fd_)
            {
                accept_connections();
            }
            else
            {
                auto it = connections_.find(fd);
                if (it != connections_.end())
                {
                    it->second->handle_events(events[i].events);
                }
                else
                {
                    close(fd);
                }
            }
        }
    }
}

void EventLoop::accept_connections()
{
    while (true)
    {
        struct sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        int client_fd = accept4(listen_fd_, (struct sockaddr *)&client_addr, &len, SOCK_NONBLOCK);
        if (client_fd == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            perror("accept4");
            break;
        }

        auto conn = std::make_shared<Connection>(client_fd, this, Connection::Type::CLIENT);
        conn->set_client_ip(inet_ntoa(client_addr.sin_addr));
        connections_[client_fd] = conn;
        add_event(client_fd, EPOLLIN | EPOLLET, conn);
    }
}

void EventLoop::add_event(int fd, uint32_t events, std::shared_ptr<Connection> conn)
{
    struct epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    if (conn)
        connections_[fd] = conn;
    if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        perror("epoll_ctl add");
    }
}

void EventLoop::update_events(int fd, uint32_t events)
{
    struct epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev) == -1)
    {
        perror("epoll_ctl mod");
    }
}

void EventLoop::remove_event(int fd)
{
    epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
    connections_.erase(fd);
}

void EventLoop::stop() { running_ = false; }