#pragma once
#include "Config.hpp"
#include <chrono>
#include <unordered_map>
#include <string>

class RateLimiter
{
public:
    explicit RateLimiter(const RateLimiterConfig &cfg);
    bool allow(const std::string &key);

private:
    struct Bucket
    {
        double tokens = 0.0;
        std::chrono::steady_clock::time_point last_refill = std::chrono::steady_clock::now();
    };
    RateLimiterConfig cfg_;
    std::unordered_map<std::string, Bucket> buckets_;
};