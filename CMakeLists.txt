cmake_minimum_required(VERSION 3.14)
project(proxy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
if(ENABLE_TSAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

add_executable(proxy
    src/main.cpp
    src/socket_utils.cpp
    src/epoll_manager.cpp
    src/connection.cpp
    src/http_parser.cpp
    src/rate_limiter.cpp
    src/load_balancer.cpp
    src/health_checker.cpp
    src/proxy_server.cpp
)

target_link_libraries(proxy PRIVATE Threads::Threads)