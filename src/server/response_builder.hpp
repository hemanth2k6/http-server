#pragma once

#include <string>
#include <vector>
#include <unordered_map>

class response_builder
{
public:
    std::string build_ok(const std::string &content, const std::string &mime_type);
    std::string build_head_ok(size_t content_length, const std::string &mime_type);
    std::string build_error(int status_code, const std::string &message);
    std::string build_not_found();

private:
    std::string build_status_line(int code, const std::string &message);
    std::string build_headers(const std::unordered_map<std::string, std::string> &headers);
    std::string get_status_message(int code);
};