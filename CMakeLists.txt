cmake_minimum_required(VERSION 3.10)
project(MultiThreadedHTTPServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCE_FILES
    src/main.cpp
    src/server/http_server.cpp
    src/server/request_parser.cpp
    src/server/response_builder.cpp
    src/server/file_handler.cpp
    src/server/thread_pool.cpp
    src/utils/socket_utils.cpp
)

add_executable(httpserver ${SOURCE_FILES})

target_include_directories(httpserver PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

find_package(Threads REQUIRED)
target_link_libraries(httpserver Threads::Threads)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Wextra -O2")

enable_testing()

add_executable(test_parser tests/test_parser.cpp src/server/request_parser.cpp src/server/response_builder.cpp src/server/thread_pool.cpp)
target_include_directories(test_parser PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_parser Threads::Threads)

add_executable(test_server tests/test_server.cpp src/server/http_server.cpp src/server/request_parser.cpp src/server/response_builder.cpp src/server/file_handler.cpp src/server/thread_pool.cpp src/utils/socket_utils.cpp)
target_include_directories(test_server PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_server Threads::Threads)