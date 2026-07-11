
#include "socket_utils.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>

namespace socket_utils
{

    bool set_nonblocking(int fd)
    {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1)
            return false;
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
    }

    int create_listen_socket(int port)
    {
        int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd < 0)
            throw std::runtime_error("socket creation failed");

        int opt = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        if (bind(listen_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
        {
            close(listen_fd);
            throw std::runtime_error("bind failed");
        }
        if (listen(listen_fd, SOMAXCONN) < 0)
        {
            close(listen_fd);
            throw std::runtime_error("listen failed");
        }
        return listen_fd;
    }

    bool set_socket_timeout(int fd, int send_timeout_sec, int recv_timeout_sec)
    {
        struct timeval tv;
        tv.tv_sec = send_timeout_sec;
        tv.tv_usec = 0;
        if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
            return false;
        tv.tv_sec = recv_timeout_sec;
        if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
            return false;
        return true;
    }

}