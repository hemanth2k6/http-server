#pragma once
#include "Config.hpp"
#include <vector>
#include <atomic>

class LoadBalancer
{
public:
    explicit LoadBalancer(const std::vector<Backend> &backends);

    Backend *next();
    void mark_failure(Backend *b);
    void mark_success(Backend *b);

private:
    std::vector<Backend> backends_;
    std::atomic<size_t> counter_{0};
};