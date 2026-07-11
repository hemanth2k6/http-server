#pragma once
#include <vector>
#include <string>
#include <atomic>
#include <optional>

struct BackendInfo
{
    std::string host;
    int port;
    std::atomic<bool> healthy;

    BackendInfo() : host{}, port{0}, healthy{false} {}
    BackendInfo(const std::string &h, int p, bool hb) : host(h), port(p), healthy(hb) {}

    BackendInfo(const BackendInfo &other)
        : host(other.host), port(other.port), healthy(other.healthy.load()) {}
    BackendInfo &operator=(const BackendInfo &other)
    {
        if (this != &other)
        {
            host = other.host;
            port = other.port;
            healthy.store(other.healthy.load());
        }
        return *this;
    }

    BackendInfo(BackendInfo &&other) noexcept
        : host(std::move(other.host)), port(other.port), healthy(other.healthy.load()) {}
    BackendInfo &operator=(BackendInfo &&other) noexcept
    {
        if (this != &other)
        {
            host = std::move(other.host);
            port = other.port;
            healthy.store(other.healthy.load());
        }
        return *this;
    }
};

class LoadBalancer
{
public:
    explicit LoadBalancer(const std::vector<BackendInfo> &backends);
    std::optional<BackendInfo> get_backend();

private:
    std::vector<BackendInfo> backends_;
    size_t next_;
};