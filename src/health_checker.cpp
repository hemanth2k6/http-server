
#include "health_checker.hpp"
#include "socket_utils.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <chrono>

HealthChecker::HealthChecker(std::vector<BackendInfo> &backends, int interval_sec)
    : backends_(backends), interval_sec_(interval_sec)
{
    thread_ = std::thread(&HealthChecker::run, this);
}

HealthChecker::~HealthChecker()
{
    stop();
    if (thread_.joinable())
        thread_.join();
}

void HealthChecker::stop()
{
    stop_flag_ = true;
}

void HealthChecker::run()
{
    while (!stop_flag_.load())
    {
        for (auto &backend : backends_)
        {
            bool healthy = false;
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0)
                continue;
            socket_utils::set_socket_timeout(sock, 2, 2);
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(backend.port);
            if (inet_pton(AF_INET, backend.host.c_str(), &addr.sin_addr) <= 0)
            {
                close(sock);
                backend.healthy.store(false);
                continue;
            }
            if (connect(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
            {
                close(sock);
                backend.healthy.store(false);
                continue;
            }
            std::string health_request = "GET /health HTTP/1.0\r\nHost: " + backend.host + "\r\nConnection: close\r\n\r\n";
            ssize_t sent = send(sock, health_request.c_str(), health_request.size(), 0);
            if (sent <= 0)
            {
                close(sock);
                backend.healthy.store(false);
                continue;
            }
            char buf[128];
            ssize_t recvd = recv(sock, buf, sizeof(buf), 0);
            if (recvd > 0)
                healthy = true;
            close(sock);
            backend.healthy.store(healthy);
        }
        std::this_thread::sleep_for(std::chrono::seconds(interval_sec_));
    }
}