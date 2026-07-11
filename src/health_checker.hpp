
#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include "load_balancer.hpp"

class HealthChecker
{
public:
    HealthChecker(std::vector<BackendInfo> &backends, int interval_sec);
    ~HealthChecker();
    void stop();

private:
    void run();
    std::vector<BackendInfo> &backends_;
    int interval_sec_;
    std::atomic<bool> stop_flag_{false};
    std::thread thread_;
};