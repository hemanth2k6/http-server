#include "UpstreamPool.hpp"
#include "Connection.hpp"
#include "EventLoop.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

UpstreamPool::UpstreamPool(EventLoop *loop) : loop_(loop) {}

std::shared_ptr<Connection> UpstreamPool::get_or_create(Backend *backend)
{
    auto &pool = pools_[backend];
    if (!pool.empty())
    {
        auto conn = pool.back();
        pool.pop_back();
        conn->reset_state();
        return conn;
    }

    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd == -1)
        return nullptr;

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (connect(fd, (struct sockaddr *)&backend->addr, sizeof(backend->addr)) == -1)
    {
        if (errno != EINPROGRESS)
        {
            close(fd);
            return nullptr;
        }
    }

    auto conn = std::make_shared<Connection>(fd, loop_, Connection::Type::UPSTREAM);
    conn->set_backend(backend);
    loop_->add_event(fd, EPOLLOUT | EPOLLIN | EPOLLET, conn);
    return conn;
}

void UpstreamPool::recycle(std::shared_ptr<Connection> conn)
{
    if (!conn || !conn->is_alive())
        return;
    Backend *b = conn->get_backend();
    if (b)
    {
        pools_[b].push_back(conn);
    }
}