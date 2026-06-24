#pragma once

#include <string>
#include <netinet/in.h>

class socket_utils
{
public:
    static bool set_nonblocking(int fd);
    static std::string get_client_ip(struct sockaddr_in addr);
    static int create_tcp_socket();
    static bool bind_socket(int fd, int port);
    static bool listen_socket(int fd, int backlog = SOMAXCONN);
};