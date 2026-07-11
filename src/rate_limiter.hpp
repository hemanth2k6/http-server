
#pragma once
#include <unordered_map>
#include <string>
#include <chrono>

class RateLimiter
{
public:
    RateLimiter(double rate, double burst);
    bool allow(const std::string &ip);

private:
    double rate_;
    double burst_;
    using Clock = std::chrono::steady_clock;
    struct Bucket
    {
        double tokens;
        Clock::time_point last_refill;
    };
    std::unordered_map<std::string, Bucket> buckets_;
};