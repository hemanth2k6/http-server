#include "server/http_server.hpp"
#include <iostream>
#include <csignal>
#include <atomic>

std::atomic<bool> running(true);

void signal_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
    {
        running = false;
    }
}

int main(int argc, char *argv[])
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    int port = 8080;
    int thread_count = 8;
    std::string doc_root = "./www";

    if (argc >= 2)
        port = std::atoi(argv[1]);
    if (argc >= 3)
        thread_count = std::atoi(argv[2]);
    if (argc >= 4)
        doc_root = argv[3];

    try
    {
        http_server server(port, thread_count, doc_root);
        server.start();

        while (running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        server.stop();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}