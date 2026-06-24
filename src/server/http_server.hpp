#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <netinet/in.h>
#include "thread_pool.hpp"
#include "request_parser.hpp"
#include "response_builder.hpp"
#include "file_handler.hpp"

class http_server
{
public:
    http_server(int port, int thread_count, const std::string &doc_root);
    ~http_server();

    void start();
    void stop();

private:
    void accept_connections();
    void handle_client(int client_fd);

    int port_;
    int thread_count_;
    std::string doc_root_;
    int server_fd_;
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> accept_thread_;
    std::unique_ptr<thread_pool> pool_;
    struct sockaddr_in address_;

    request_parser parser_;
    response_builder builder_;
    file_handler file_handler_;
};