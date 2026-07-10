#include "LoadBalancer.hpp"

LoadBalancer::LoadBalancer(const std::vector<Backend> &backends) : backends_(backends) {}

Backend *LoadBalancer::next()
{
    if (backends_.empty())
        return nullptr;
    size_t size = backends_.size();
    size_t start = counter_.fetch_add(1) % size;
    for (size_t i = 0; i < size; ++i)
    {
        size_t idx = (start + i) % size;
        if (backends_[idx].healthy)
        {
            return &backends_[idx];
        }
    }
    return nullptr;
}

void LoadBalancer::mark_failure(Backend *b)
{
    if (!b)
        return;
    b->failures++;
    if (b->failures > 3)
        b->healthy = false;
}

void LoadBalancer::mark_success(Backend *b)
{
    if (!b)
        return;
    b->failures = 0;
    b->healthy = true;
}