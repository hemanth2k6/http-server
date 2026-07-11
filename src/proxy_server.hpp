
#pragma once
#include <memory>
#include <unordered_map>
#include <vector>
#include "epoll_manager.hpp"
#include "rate_limiter.hpp"
#include "load_balancer.hpp"
#include "health_checker.hpp"

class ClientConnection;

class ProxyServer
{
public:
    ProxyServer(int listen_port, const std::vector<BackendInfo> &backends, double rate, double burst, int health_check_interval);
    ~ProxyServer();
    void run();
    RateLimiter &rate_limiter() { return rate_limiter_; }
    LoadBalancer &load_balancer() { return load_balancer_; }

private:
    int listen_fd_;
    EpollManager epoll_;
    RateLimiter rate_limiter_;
    LoadBalancer load_balancer_;
    HealthChecker health_checker_;
    std::unordered_map<int, std::unique_ptr<ClientConnection>> clients_;
    bool running_;
};