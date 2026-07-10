#pragma once
#include "Config.hpp"
#include "LoadBalancer.hpp"
#include "RateLimiter.hpp"
#include "UpstreamPool.hpp"
#include <unordered_map>
#include <memory>
#include <atomic>
#include <sys/epoll.h>

class Connection;

class EventLoop
{
public:
    explicit EventLoop(const Config &cfg);
    ~EventLoop();

    void run();
    void stop();

    void add_event(int fd, uint32_t events, std::shared_ptr<Connection> conn);
    void update_events(int fd, uint32_t events);
    void remove_event(int fd);

    RateLimiter &get_rate_limiter() { return limiter_; }
    LoadBalancer &get_load_balancer() { return lb_; }
    UpstreamPool &get_upstream_pool() { return pool_; }

private:
    Config cfg_;
    int epfd_;
    std::atomic<bool> running_{true};
    LoadBalancer lb_;
    RateLimiter limiter_;
    UpstreamPool pool_;
    std::unordered_map<int, std::shared_ptr<Connection>> connections_;
    int listen_fd_ = -1;

    void accept_connections();
    void setup_listener();
};