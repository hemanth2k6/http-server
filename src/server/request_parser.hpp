#pragma once

#include <string>
#include <unordered_map>

struct http_request
{
    std::string method;
    std::string uri;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    bool is_valid = false;
};

class request_parser
{
public:
    http_request parse(const std::string &raw_data);

private:
    void parse_request_line(const std::string &line, http_request &request);
    void parse_header(const std::string &line, http_request &request);
};