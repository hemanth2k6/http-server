cmake_minimum_required(VERSION 3.10)
project(RainbowProxy)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -pthread")

include_directories(include)

add_executable(proxy
    src/main.cpp
    src/Connection.cpp
    src/EventLoop.cpp
    src/HttpParser.cpp
    src/LoadBalancer.cpp
    src/RateLimiter.cpp
    src/UpstreamPool.cpp
)

target_link_libraries(proxy pthread)