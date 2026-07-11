
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

class HttpRequestParser
{
public:
    enum class ParseResult
    {
        INCOMPLETE,
        COMPLETE,
        ERROR
    };
    HttpRequestParser();
    ParseResult parse(const std::string &data);
    std::string reconstruct_request() const;
    size_t get_header_end() const { return header_end_pos_; }
    size_t get_content_length() const { return content_length_; }

private:
    enum class State
    {
        PARSE_REQUEST_LINE,
        PARSE_HEADERS,
        PARSE_BODY,
        COMPLETE
    };
    State state_;
    std::string method_;
    std::string uri_;
    std::string version_;
    std::vector<std::pair<std::string, std::string>> headers_;
    size_t content_length_;
    size_t body_read_;
    size_t header_end_pos_;
    bool headers_complete_;
};