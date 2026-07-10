#pragma once
#include "Config.hpp"
#include <memory>
#include <vector>
#include <unordered_map>

class Connection;
class EventLoop;

class UpstreamPool
{
public:
    explicit UpstreamPool(EventLoop *loop);

    std::shared_ptr<Connection> get_or_create(Backend *backend);
    void recycle(std::shared_ptr<Connection> conn);

private:
    EventLoop *loop_;
    std::unordered_map<Backend *, std::vector<std::shared_ptr<Connection>>> pools_;
};