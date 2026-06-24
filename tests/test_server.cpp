#include "server/http_server.hpp"
#include <iostream>
#include <thread>
#include <chrono>

void test_server_lifecycle()
{
    try
    {
        http_server server(8081, 2, "./www");
        server.start();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        server.stop();
        std::cout << "Server lifecycle test passed!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Server lifecycle test failed: " << e.what() << std::endl;
    }
}

int main()
{
    test_server_lifecycle();
    return 0;
}