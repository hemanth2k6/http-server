
#include "load_balancer.hpp"

LoadBalancer::LoadBalancer(const std::vector<BackendInfo> &backends)
    : backends_(backends), next_(0) {}

std::optional<BackendInfo> LoadBalancer::get_backend()
{
    if (backends_.empty())
        return std::nullopt;
    size_t count = backends_.size();
    for (size_t i = 0; i < count; ++i)
    {
        size_t index = (next_ + i) % count;
        if (backends_[index].healthy.load())
        {
            next_ = (index + 1) % count;
            BackendInfo info = backends_[index];
            return info;
        }
    }
    return std::nullopt;
}