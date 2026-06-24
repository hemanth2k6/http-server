#include "utils/socket_utils.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

bool socket_utils::set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

std::string socket_utils::get_client_ip(struct sockaddr_in addr)
{
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, INET_ADDRSTRLEN);
    return std::string(ip);
}

int socket_utils::create_tcp_socket()
{
    return socket(AF_INET, SOCK_STREAM, 0);
}

bool socket_utils::bind_socket(int fd, int port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    return bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0;
}

bool socket_utils::listen_socket(int fd, int backlog)
{
    return listen(fd, backlog) == 0;
}