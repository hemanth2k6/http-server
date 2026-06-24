#include "server/file_handler.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <algorithm>

file_handler::file_handler(const std::string &doc_root) : doc_root_(doc_root)
{
    init_mime_types();
}

void file_handler::init_mime_types()
{
    mime_map_[".html"] = "text/html";
    mime_map_[".htm"] = "text/html";
    mime_map_[".css"] = "text/css";
    mime_map_[".js"] = "application/javascript";
    mime_map_[".json"] = "application/json";
    mime_map_[".png"] = "image/png";
    mime_map_[".jpg"] = "image/jpeg";
    mime_map_[".jpeg"] = "image/jpeg";
    mime_map_[".gif"] = "image/gif";
    mime_map_[".svg"] = "image/svg+xml";
    mime_map_[".txt"] = "text/plain";
    mime_map_[".xml"] = "application/xml";
    mime_map_[".pdf"] = "application/pdf";
    mime_map_[".zip"] = "application/zip";
    mime_map_[".gz"] = "application/gzip";
}

bool file_handler::read_file(const std::string &path, std::string &content)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
    {
        return false;
    }

    if (!S_ISREG(st.st_mode))
    {
        return false;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    content = buffer.str();
    return true;
}

std::string file_handler::get_mime_type(const std::string &path)
{
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos)
    {
        return "application/octet-stream";
    }

    std::string ext = path.substr(dot_pos);
    auto it = mime_map_.find(ext);
    if (it != mime_map_.end())
    {
        return it->second;
    }

    return "application/octet-stream";
}