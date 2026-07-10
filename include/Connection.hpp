#pragma once
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <sys/epoll.h>

class EventLoop;
struct Backend;

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    enum class Type
    {
        CLIENT,
        UPSTREAM
    };
    enum class State
    {
        CONNECTING,
        READING_REQUEST,
        FORWARDING_REQUEST,
        READING_RESPONSE,
        SENDING_RESPONSE,
        CLOSING
    };

    Connection(int fd, EventLoop *loop, Type type);
    ~Connection();

    void set_upstream(std::shared_ptr<Connection> upstream);
    void set_backend(Backend *backend);
    void set_client_ip(const std::string &ip);
    void set_request(std::string &&req);
    void reset_state();

    void handle_events(uint32_t events);
    void write_response(const std::string &response);
    void schedule_close();
    bool is_alive() const;
    Backend *get_backend() const;
    int fd() const { return fd_; }

private:
    int fd_;
    EventLoop *loop_;
    Type type_;
    State state_;
    std::weak_ptr<Connection> upstream_;
    Backend *backend_;

    std::vector<char> read_buffer_;
    std::vector<char> write_buffer_;
    size_t write_offset_ = 0;
    std::string request_;
    std::string client_ip_;
    bool connected_ = false;

    void handle_read();
    void handle_write();
    void handle_error();
    bool try_forward_request();
    void close_connection();
    void send_http_error(int code, const std::string &msg);
};