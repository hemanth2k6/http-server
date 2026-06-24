#include "server/response_builder.hpp"
#include <sstream>
#include <ctime>

std::string response_builder::build_ok(const std::string &content, const std::string &mime_type)
{
    std::unordered_map<std::string, std::string> headers;
    headers["Content-Type"] = mime_type;
    headers["Content-Length"] = std::to_string(content.size());
    headers["Connection"] = "close";
    headers["Server"] = "MultiThreadedHTTPServer/1.0";

    time_t now = time(nullptr);
    headers["Date"] = std::string(ctime(&now));
    headers["Date"].pop_back();

    std::string response = build_status_line(200, "OK") + "\r\n";
    response += build_headers(headers) + "\r\n";
    response += content;
    return response;
}

std::string response_builder::build_head_ok(size_t content_length, const std::string &mime_type)
{
    std::unordered_map<std::string, std::string> headers;
    headers["Content-Type"] = mime_type;
    headers["Content-Length"] = std::to_string(content_length);
    headers["Connection"] = "close";
    headers["Server"] = "MultiThreadedHTTPServer/1.0";

    time_t now = time(nullptr);
    headers["Date"] = std::string(ctime(&now));
    headers["Date"].pop_back();

    std::string response = build_status_line(200, "OK") + "\r\n";
    response += build_headers(headers) + "\r\n";
    return response;
}

std::string response_builder::build_error(int status_code, const std::string &message)
{
    std::string status_message = get_status_message(status_code);
    std::string body = "<html><body><h1>" + std::to_string(status_code) + " " + status_message + "</h1><p>" + message + "</p></body></html>";

    std::unordered_map<std::string, std::string> headers;
    headers["Content-Type"] = "text/html";
    headers["Content-Length"] = std::to_string(body.size());
    headers["Connection"] = "close";
    headers["Server"] = "MultiThreadedHTTPServer/1.0";

    std::string response = build_status_line(status_code, status_message) + "\r\n";
    response += build_headers(headers) + "\r\n";
    response += body;
    return response;
}

std::string response_builder::build_not_found()
{
    return build_error(404, "The requested resource was not found on this server.");
}

std::string response_builder::build_status_line(int code, const std::string &message)
{
    return "HTTP/1.1 " + std::to_string(code) + " " + message;
}

std::string response_builder::build_headers(const std::unordered_map<std::string, std::string> &headers)
{
    std::string result;
    for (const auto &[key, value] : headers)
    {
        result += key + ": " + value + "\r\n";
    }
    return result;
}

std::string response_builder::get_status_message(int code)
{
    switch (code)
    {
    case 200:
        return "OK";
    case 400:
        return "Bad Request";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 500:
        return "Internal Server Error";
    default:
        return "Unknown";
    }
}