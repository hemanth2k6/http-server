
#include "rate_limiter.hpp"
#include <algorithm>

RateLimiter::RateLimiter(double rate, double burst) : rate_(rate), burst_(burst) {}

bool RateLimiter::allow(const std::string &ip)
{
    auto now = Clock::now();
    auto it = buckets_.find(ip);
    if (it == buckets_.end())
    {
        Bucket b{burst_, now};
        it = buckets_.emplace(ip, b).first;
    }
    else
    {
        auto &b = it->second;
        double elapsed = std::chrono::duration<double>(now - b.last_refill).count();
        b.tokens = std::min(burst_, b.tokens + elapsed * rate_);
        b.last_refill = now;
    }
    if (it->second.tokens >= 1.0)
    {
        it->second.tokens -= 1.0;
        return true;
    }
    return false;
}