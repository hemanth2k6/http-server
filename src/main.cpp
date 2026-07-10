#include "EventLoop.hpp"
#include "Config.hpp"
#include <csignal>
#include <iostream>
#include <arpa/inet.h>

static std::unique_ptr<EventLoop> g_loop;

void signal_handler(int)
{
    std::cout << "\nShutting down gracefully...\n";
    if (g_loop)
        g_loop->stop();
}

Config build_config()
{
    Config cfg;
    cfg.port = 8080;

    Backend b1, b2, b3;
    b1.host = "127.0.0.1";
    b1.port = 9001;
    b2.host = "127.0.0.1";
    b2.port = 9002;
    b3.host = "127.0.0.1";
    b3.port = 9003;

    auto fill_addr = [](Backend &b)
    {
        b.addr.sin_family = AF_INET;
        b.addr.sin_port = htons(b.port);
        inet_pton(AF_INET, b.host.c_str(), &b.addr.sin_addr);
    };
    fill_addr(b1);
    fill_addr(b2);
    fill_addr(b3);

    cfg.backends.push_back(b1);
    cfg.backends.push_back(b2);
    cfg.backends.push_back(b3);

    cfg.rate_limit.rate = 10.0;
    cfg.rate_limit.capacity = 20;

    return cfg;
}

int main()
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    Config cfg = build_config();
    g_loop = std::make_unique<EventLoop>(cfg);

    std::cout << "Proxy listening on port " << cfg.port << "\n";
    std::cout << "Backends: ";
    for (auto &b : cfg.backends)
        std::cout << b.host << ":" << b.port << " ";
    std::cout << "\nRate limit: " << cfg.rate_limit.rate << " req/s, burst " << cfg.rate_limit.capacity << "\n";

    g_loop->run();
    return 0;
}