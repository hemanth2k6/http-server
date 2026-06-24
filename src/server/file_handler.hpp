#pragma once

#include <string>
#include <unordered_map>

class file_handler
{
public:
    file_handler(const std::string &doc_root);
    bool read_file(const std::string &path, std::string &content);
    std::string get_mime_type(const std::string &path);

private:
    std::string doc_root_;
    std::unordered_map<std::string, std::string> mime_map_;
    void init_mime_types();
};