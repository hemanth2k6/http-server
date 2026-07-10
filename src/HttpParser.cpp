#include "HttpParser.hpp"
#include <cstring>
#include <algorithm>

std::optional<HttpRequest> HttpParser::parse(const std::vector<char> &buffer)
{
    std::string data(buffer.begin(), buffer.end());
    size_t he = data.find("\r\n\r\n");
    if (he == std::string::npos)
        return std::nullopt;

    HttpRequest req;
    req.headers_end = he + 4;
    req.complete = false;

    size_t cl_pos = data.find("Content-Length:");
    if (cl_pos != std::string::npos)
    {
        size_t start = data.find(':', cl_pos) + 1;
        size_t end = data.find("\r\n", start);
        if (end != std::string::npos)
        {
            req.content_length = std::stoul(data.substr(start, end - start));
        }
    }

    size_t body_received = data.size() - req.headers_end;
    if (body_received >= req.content_length)
    {
        req.complete = true;
    }

    return req;
}