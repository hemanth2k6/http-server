
#pragma once
#include <cstdint>

namespace socket_utils
{
    bool set_nonblocking(int fd);
    int create_listen_socket(int port);
    bool set_socket_timeout(int fd, int send_timeout_sec, int recv_timeout_sec);
}