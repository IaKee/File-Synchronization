#pragma once

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>
#include <sys/stat.h>

# include "json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace file_manager
{
    class FileManager
    {
        public:
            bool open_file(const std::string& file_path, const std::string& tag);
            bool path_exists(std::string path);
            bool close_file(const std::string& tag);
            bool is_file_open(const std::string& tag);
            bool delete_file(const std::string& tag);
            std::string calculate_checksum(const std::string& tag);
            bool compare_files(const std::string& tag1, const std::string& tag2);
            int get_file_size(const std::string& tag);
            std::string get_file_path(const std::string& tag);
            std::string find_tag_by_path(const std::string& file_path);
            int get_last_modified_time(const std::string& tag);
            std::string get_last_modified_date(const std::string& tag);
            bool create_directory(const std::string& path);
            bool check_permissions(const std::string& path);
            bool create_empty_file(const std::string& path);
            json get_json_contents(const std::string& path);

        private:
            bool dummy_;

            std::unordered_map<std::string, std::pair<std::string, std::ifstream>> files_;
            std::string generate_tag(const std::string& file_path);
    };
}