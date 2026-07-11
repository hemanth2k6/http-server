
#include "http_parser.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

HttpRequestParser::HttpRequestParser()
    : state_(State::PARSE_REQUEST_LINE), content_length_(0), body_read_(0),
      header_end_pos_(0), headers_complete_(false) {}

HttpRequestParser::ParseResult HttpRequestParser::parse(const std::string &data)
{
    if (!headers_complete_)
    {
        std::string_view view(data);
        size_t header_end = view.find("\r\n\r\n");
        if (header_end == std::string::npos)
            return ParseResult::INCOMPLETE;
        std::string headers_block(view.substr(0, header_end));
        std::stringstream ss(headers_block);
        std::string line;
        bool first_line = true;
        while (std::getline(ss, line, '\r'))
        {
            if (!line.empty() && line[0] == '\n')
                line.erase(0, 1);
            if (line.empty())
                continue;
            if (first_line)
            {
                size_t sp1 = line.find(' ');
                size_t sp2 = line.find(' ', sp1 + 1);
                if (sp1 == std::string::npos || sp2 == std::string::npos)
                    return ParseResult::ERROR;
                method_ = line.substr(0, sp1);
                uri_ = line.substr(sp1 + 1, sp2 - sp1 - 1);
                version_ = line.substr(sp2 + 1);
                first_line = false;
            }
            else
            {
                size_t colon = line.find(':');
                if (colon != std::string::npos)
                {
                    std::string key = line.substr(0, colon);
                    std::string value = line.substr(colon + 1);
                    if (!value.empty() && value[0] == ' ')
                        value.erase(0, 1);
                    headers_.push_back({key, value});
                }
            }
        }
        auto it = std::find_if(headers_.begin(), headers_.end(), [](const auto &p)
                               {
            std::string lower = p.first;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            return lower == "content-length"; });
        if (it != headers_.end())
        {
            content_length_ = std::stoul(it->second);
        }
        else
        {
            content_length_ = 0;
        }
        header_end_pos_ = header_end + 4;
        headers_complete_ = true;
        state_ = (content_length_ > 0) ? State::PARSE_BODY : State::COMPLETE;
    }
    if (state_ == State::PARSE_BODY)
    {
        size_t data_after_headers = data.size() - header_end_pos_;
        if (data_after_headers >= content_length_)
        {
            body_read_ = content_length_;
            state_ = State::COMPLETE;
        }
        else
        {
            return ParseResult::INCOMPLETE;
        }
    }
    if (state_ == State::COMPLETE)
        return ParseResult::COMPLETE;
    return ParseResult::INCOMPLETE;
}

std::string HttpRequestParser::reconstruct_request() const
{
    std::string req;
    req += method_ + " " + uri_ + " " + version_ + "\r\n";
    for (const auto &h : headers_)
    {
        req += h.first + ": " + h.second + "\r\n";
    }
    return req;
}