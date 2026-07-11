
#pragma once
#include <sys/epoll.h>
#include <vector>
#include <stdexcept>
#include <unistd.h>

class EpollManager
{
public:
    EpollManager();
    ~EpollManager();
    void add_fd(int fd, uint32_t events, void *ptr);
    void mod_fd(int fd, uint32_t events, void *ptr);
    void del_fd(int fd);
    int wait(std::vector<epoll_event> &events, int timeout_ms);

private:
    int epoll_fd_;
};