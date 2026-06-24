#include "server/request_parser.hpp"
#include <sstream>
#include <algorithm>

http_request request_parser::parse(const std::string &raw_data)
{
    http_request request;
    std::istringstream stream(raw_data);
    std::string line;
    bool has_request_line = false;

    while (std::getline(stream, line))
    {
        if (line.back() == '\r')
            line.pop_back();

        if (!has_request_line)
        {
            parse_request_line(line, request);
            has_request_line = true;
            if (!request.is_valid)
                return request;
        }
        else if (line.empty())
        {
            break;
        }
        else
        {
            parse_header(line, request);
        }
    }

    size_t body_pos = raw_data.find("\r\n\r\n");
    if (body_pos != std::string::npos)
    {
        request.body = raw_data.substr(body_pos + 4);
    }

    return request;
}

void request_parser::parse_request_line(const std::string &line, http_request &request)
{
    std::istringstream iss(line);
    if (!(iss >> request.method >> request.uri >> request.version))
    {
        request.is_valid = false;
        return;
    }

    if (request.method != "GET" && request.method != "HEAD" &&
        request.method != "POST" && request.method != "PUT" &&
        request.method != "DELETE")
    {
        request.is_valid = false;
        return;
    }

    if (request.version != "HTTP/1.1")
    {
        request.is_valid = false;
        return;
    }

    if (request.uri.empty() || request.uri[0] != '/')
    {
        request.is_valid = false;
        return;
    }

    request.is_valid = true;
}

void request_parser::parse_header(const std::string &line, http_request &request)
{
    size_t colon_pos = line.find(':');
    if (colon_pos != std::string::npos)
    {
        std::string key = line.substr(0, colon_pos);
        std::string value = line.substr(colon_pos + 1);

        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        request.headers[key] = value;
    }
}