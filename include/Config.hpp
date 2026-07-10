#pragma once
#include <vector>
#include <string>
#include <netinet/in.h>

struct Backend
{
    std::string host;
    int port;
    bool healthy = true;
    int failures = 0;
    struct sockaddr_in addr;
};

struct RateLimiterConfig
{
    double rate = 10.0;
    int capacity = 20;
};

struct Config
{
    int port = 8080;
    std::vector<Backend> backends;
    RateLimiterConfig rate_limit;
};