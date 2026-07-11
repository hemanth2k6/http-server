
#include "epoll_manager.hpp"

EpollManager::EpollManager()
{
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0)
        throw std::runtime_error("epoll_create1 failed");
}

EpollManager::~EpollManager()
{
    if (epoll_fd_ >= 0)
        close(epoll_fd_);
}

void EpollManager::add_fd(int fd, uint32_t events, void *ptr)
{
    epoll_event ev;
    ev.events = events;
    ev.data.ptr = ptr;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0)
        throw std::runtime_error("epoll_ctl add failed");
}

void EpollManager::mod_fd(int fd, uint32_t events, void *ptr)
{
    epoll_event ev;
    ev.events = events;
    ev.data.ptr = ptr;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) < 0)
        throw std::runtime_error("epoll_ctl mod failed");
}

void EpollManager::del_fd(int fd)
{
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
}

int EpollManager::wait(std::vector<epoll_event> &events, int timeout_ms)
{
    const int MAX_EVENTS = 1024;
    events.resize(MAX_EVENTS);
    int nfds = epoll_wait(epoll_fd_, events.data(), MAX_EVENTS, timeout_ms);
    events.resize(nfds > 0 ? nfds : 0);
    return nfds;
}