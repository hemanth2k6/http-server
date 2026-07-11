
#include "proxy_server.hpp"
#include "load_balancer.hpp"
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int listen_port = 8080;
    std::vector<BackendInfo> backends;
    double rate = 100.0 / 60.0;
    double burst = 10.0;
    int health_interval = 10;
    std::string backend_str;
    int opt;
    while ((opt = getopt(argc, argv, "p:b:r:t:h:")) != -1)
    {
        switch (opt)
        {
        case 'p':
            listen_port = std::stoi(optarg);
            break;
        case 'b':
            backend_str = optarg;
            break;
        case 'r':
            rate = std::stod(optarg);
            break;
        case 't':
            burst = std::stod(optarg);
            break;
        case 'h':
            health_interval = std::stoi(optarg);
            break;
        default:
            std::cerr << "Usage: " << argv[0] << " -p <port> -b <host1:port1,host2:port2> -r <rate> -t <burst> -h <health_interval>\n";
            return 1;
        }
    }
    if (backend_str.empty())
    {
        std::cerr << "Backend list required (-b)\n";
        return 1;
    }
    size_t pos = 0;
    while (pos < backend_str.size())
    {
        size_t comma = backend_str.find(',', pos);
        std::string token = backend_str.substr(pos, comma - pos);
        size_t colon = token.find(':');
        if (colon != std::string::npos)
        {
            std::string host = token.substr(0, colon);
            int port = std::stoi(token.substr(colon + 1));
            BackendInfo info;
            info.host = host;
            info.port = port;
            info.healthy.store(true);
            backends.push_back(std::move(info));
        }
        if (comma == std::string::npos)
            break;
        pos = comma + 1;
    }
    try
    {
        ProxyServer server(listen_port, backends, rate, burst, health_interval);
        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}