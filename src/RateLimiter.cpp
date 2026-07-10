#include "RateLimiter.hpp"
#include <algorithm>

RateLimiter::RateLimiter(const RateLimiterConfig &cfg) : cfg_(cfg) {}

bool RateLimiter::allow(const std::string &key)
{
    using namespace std::chrono;
    auto now = steady_clock::now();
    auto &bucket = buckets_[key];

    double elapsed = duration<double>(now - bucket.last_refill).count();
    bucket.tokens = std::min<double>(cfg_.capacity, bucket.tokens + elapsed * cfg_.rate);
    bucket.last_refill = now;

    if (bucket.tokens >= 1.0)
    {
        bucket.tokens -= 1.0;
        return true;
    }
    return false;
}