#include "server/request_parser.hpp"
#include "server/response_builder.hpp"
#include <iostream>
#include <cassert>
#include <string>

void test_request_parser()
{
    request_parser parser;

    std::string raw = "GET /index.html HTTP/1.1\r\n"
                      "Host: localhost:8080\r\n"
                      "User-Agent: TestClient/1.0\r\n"
                      "\r\n";

    auto request = parser.parse(raw);
    assert(request.is_valid);
    assert(request.method == "GET");
    assert(request.uri == "/index.html");
    assert(request.version == "HTTP/1.1");
    assert(request.headers["Host"] == "localhost:8080");
    assert(request.headers["User-Agent"] == "TestClient/1.0");
    std::cout << "Request parser test passed!" << std::endl;
}

void test_response_builder()
{
    response_builder builder;
    std::string content = "<html><body>Test</body></html>";
    std::string response = builder.build_ok(content, "text/html");

    assert(response.find("HTTP/1.1 200 OK") != std::string::npos);
    assert(response.find("Content-Type: text/html") != std::string::npos);

    std::string expected_length = "Content-Length: " + std::to_string(content.size());
    assert(response.find(expected_length) != std::string::npos);

    assert(response.find("<html><body>Test</body></html>") != std::string::npos);
    std::cout << "Response builder test passed!" << std::endl;
}

int main()
{
    test_request_parser();
    test_response_builder();
    return 0;
}