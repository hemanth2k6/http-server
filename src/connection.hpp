
#pragma once
#include <string>
#include <memory>
#include <sys/epoll.h>
#include "http_parser.hpp"

class EpollManager;
class ProxyServer;
class BackendConnection;

class ConnectionBase
{
public:
    virtual ~ConnectionBase() = default;
    virtual void handle_event(uint32_t events) = 0;
    virtual int get_fd() const = 0;
    virtual bool is_backend() const = 0;
};

class ClientConnection : public ConnectionBase
{
public:
    ClientConnection(int fd, EpollManager &epoll, ProxyServer &server);
    ~ClientConnection() override;
    void handle_event(uint32_t events) override;
    int get_fd() const override { return fd_; }
    bool is_backend() const override { return false; }
    void on_backend_response(std::string response);
    void initiate_close();

private:
    enum class State
    {
        READING_REQUEST,
        WAITING_BACKEND,
        SENDING_RESPONSE,
        CLOSING
    };
    int fd_;
    EpollManager &epoll_;
    ProxyServer &server_;
    State state_;
    std::string read_buffer_;
    std::string write_buffer_;
    size_t write_offset_;
    HttpRequestParser parser_;
    std::unique_ptr<BackendConnection> backend_;
    bool response_ready_;

    void process_read();
    void process_write();
    void start_backend_request();
    void send_error_response(int status_code, const std::string &message);
};

class BackendConnection : public ConnectionBase
{
public:
    BackendConnection(int fd, ClientConnection *client, EpollManager &epoll, const std::string &request_data);
    ~BackendConnection() override;
    void handle_event(uint32_t events) override;
    int get_fd() const override { return fd_; }
    bool is_backend() const override { return true; }
    void start_sending();

private:
    enum class State
    {
        CONNECTING,
        SENDING_REQUEST,
        READING_RESPONSE,
        DONE
    };
    int fd_;
    EpollManager &epoll_;
    ClientConnection *client_;
    State state_;
    std::string request_data_;
    size_t send_offset_;
    std::string response_buffer_;
};