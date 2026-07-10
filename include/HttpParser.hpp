#pragma once
#include <vector>
#include <string>
#include <optional>

struct HttpRequest
{
    std::string method;
    std::string path;
    size_t content_length = 0;
    bool complete = false;
    size_t headers_end = 0;
};

class HttpParser
{
public:
    static std::optional<HttpRequest> parse(const std::vector<char> &buffer);
};