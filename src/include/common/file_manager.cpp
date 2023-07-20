#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <sys/stat.h>

namespace fs = std::filesystem;

class FileManager
{
    public:
        FileManager() {}

        bool open_file(const std::string& file_path, const std::string& tag = "")
        {  // open a file from a given path and associates with a tag
            std::ifstream file(file_path, std::ios::binary);
            if (file)
            {
                std::string file_tag = tag.empty() ? generate_tag(file_path) : tag;
                if (files_.count(file_tag) == 0)
                {
                    files_[file_tag] = {file_path, std::move(file)};
                    return true;
                }
                else
                {
                    std::cout << "Oops! It seems there's aready a file handled by the tag \"" << file_tag << "\"." << std::endl;
                }
            }
            return false;
        }
        bool close_file(const std::string& tag)
        {  // closes a file - associated with a tag
            auto it = files_.find(tag);
            if (it != files_.end())
            {
                it->second.second.close();
                files_.erase(it);
                return true;
            }
            return false;
        }
        bool is_file_open(const std::string& tag)
        {  // checks if a file is currently open - associated with a tag
            return files_.count(tag) > 0;
        }
        bool delete_file(const std::string& tag)
        {  // deletes a file - associated with a tag
            if (close_file(tag))
            {
                std::string file_path = files_[tag].first;
                return std::remove(file_path.c_str()) == 0;
            }
            return false;
        }
        std::string calculate_checksum(const std::string& tag)
        {  // calculates the checksum of the contents of a file - associated with a tag
            auto it = files_.find(tag);
            if (it != files_.end())
            {
                std::ifstream& file = it->second.second;
                std::stringstream checksum;
                checksum << std::hex << std::setfill('0');
                char buffer[4096];

                while (file.read(buffer, sizeof(buffer)))
                {
                    checksum << std::setw(2);
                    for (size_t i = 0; i < sizeof(buffer); i++)
                    {
                        checksum << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(buffer[i]));
                    }
                }

                checksum << std::setw(2);
                for (size_t i = 0; i < file.gcount(); i++)
                {
                    checksum << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(buffer[i]));
                }

                return checksum.str();
            }
            return "";
        }
        bool compare_files(const std::string& tag1, const std::string& tag2)
        {  // compares two files associated with two different tags
            
            // if both tags are the same
            if(tag1 == tag2)
                return true;

            auto it1 = files_.find(tag1);
            auto it2 = files_.find(tag2);
            if (it1 != files_.end() && it2 != files_.end())
            {
                std::ifstream& file1 = it1->second.second;
                std::ifstream& file2 = it2->second.second;

                if (file1.tellg() == file2.tellg())
                {
                    file1.seekg(0);
                    file2.seekg(0);

                    char buffer1[4096];
                    char buffer2[4096];

                    do
                    {
                        file1.read(buffer1, sizeof(buffer1));
                        file2.read(buffer2, sizeof(buffer2));
                        size_t count1 = file1.gcount();
                        size_t count2 = file2.gcount();
                        if (count1 != count2 || std::memcmp(buffer1, buffer2, count1) != 0)
                        {  // if any difference if found while checking file streams
                            return false;
                        }
                    } 
                    while (file1 && file2);

                    return true;
                }
            }
            return false;
        }
        int get_file_size(const std::string& tag)
        {  // retrives file size in bytes - associated with a tag
            auto it = files_.find(tag);
            if (it != files_.end())
            {
                std::ifstream& file = it->second.second;
                std::streampos current_position = file.tellg();
                file.seekg(0, std::ios::end);
                std::streampos file_size = file.tellg();
                file.seekg(current_position);
                return static_cast<int>(file_size);
            }
            return -1;
        }
        const std::string& get_file_path(const std::string& tag)
        {  // retrieves file path - associated with a tag
            static const std::string empty_path;
            auto it = files_.find(tag);
            if (it != files_.end())
            {
                return it->second.first;
            }
            return empty_path;
        }
        std::string find_tag_by_path(const std::string& file_path)
        {  // retrieves file tag, if open - given the file path
            for (const auto& pair : files_)
            {
                if (pair.second.first == file_path)
                {
                    return pair.first;
                }
            }
            return "";
        }  
        int get_last_modified_time(const std::string& tag)
        {  // retrieves the timestamp of the last modification of a file - associated with a tag
            if (files_.count(tag) > 0)
            {
                const std::string& file_path = files_[tag].first;
                if (fs::exists(file_path))
                {
                    auto last_modified_time = fs::last_write_time(file_path);
                    auto duration = last_modified_time.time_since_epoch();
                    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
                }
            }
            return -1;  // returns -1 if the tag does not exist
        }
        std::string get_last_modified_date(const std::string& tag)
        {  // retrieves the date and time of the last modification of a file - associated with a tag
            if (files_.count(tag) > 0)
            {
                const std::string& file_path = files_[tag].first;
                struct stat st;
                if (stat(file_path.c_str(), &st) == 0)
                {
                    std::time_t last_modified_time_t = st.st_mtime;
                    std::tm* modified_tm = std::localtime(&last_modified_time_t);
                    char buffer[100];
                    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", modified_tm);
                    return buffer;
                }
            }
            return "";  // Return an empty string if the tag or file does not exist
        }

    private:
        std::unordered_map<std::string, std::pair<std::string, std::ifstream>> files_;

        std::string generate_tag(const std::string& file_path)
        {  // generates an alias (tag) given a file path
            size_t last_slash_pos = file_path.find_last_of('/');
            size_t last_backslash_pos = file_path.find_last_of('\\');
            size_t last_separator_pos = std::max(last_slash_pos, last_backslash_pos);
            size_t extension_pos = file_path.find_last_of('.');
            if (last_separator_pos != std::string::npos)
            {
                last_separator_pos++;
            }
            std::string file_name = file_path.substr(last_separator_pos, extension_pos - last_separator_pos);
            return file_name;
        }
};