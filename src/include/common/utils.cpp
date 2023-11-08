// C++ Standard Libraries
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cstring>
#include <iostream>
#include <regex>
#include <string>
#include <list>
#include <queue>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <condition_variable>
#include <vector>
#include <unistd.h>

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/cryptlib.h>
#include <cryptopp/md5.h>
#include <cryptopp/hex.h>
#include <cryptopp/files.h>

// locals
#include "utils.hpp"
#include "json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

bool is_valid_username(const std::string& username) 
{
    // checks if the username is alphanumeric and does not have any symbols (including spaces)
    for(char c : username) 
    {
        if (!std::isalnum(c)) 
        {
            return false;
        }
    }
    return true;
}

bool is_valid_address(const std::string& ip_address) 
{
    // checks if given ip is valid by the following regex pattern
    std::regex pattern(R"(^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$)");
    
    if(std::regex_match(ip_address, pattern))
        return true;
    else if(ip_address == "localhost")
        return true;
    return false;
}

bool is_valid_port(int port) 
{
    // checks if the given port is in the port range
    int port_value = port;
    return port_value >= 0 && port_value <= 65535;
}

bool is_valid_path(std::string path)
{
    // checks if the informed folder does exist within the file system
    return (std::filesystem::exists(path)) ? true : false;
}

bool create_directory(const std::string& path) 
{
    return std::filesystem::create_directories(path);
}

bool create_file(const std::string& path)
{
    std::ofstream new_file(path);

    if (new_file.is_open())
    {
        new_file.close();
        return true;
    }
    return false;
}

void delete_file(const std::string& file_path)
{
    if(std::remove(file_path.c_str()) != 0) 
    {
        throw std::runtime_error("Could not delete file!");
    }
}

const char* strchar(std::string& i)
{
    return i.c_str();
}

void strcharray(std::string input_string, char* output_char_array, size_t max_size) 
{
    if(input_string.size() > (max_size - 1))
    {
        throw std::runtime_error("[UTILS] Given string \"" + input_string + "\" exceeds vector maximum size!");
    }

    size_t copied_size = std::min(input_string.size(), max_size - 1);
    strncpy(output_char_array, input_string.c_str(), copied_size);

    // fills the remaining of the array with null characters
    for (size_t i = copied_size; i < max_size; ++i) 
    {
        output_char_array[i] = '\0';
    }
}

std::string charraystr(char* char_array, size_t size)
{
    return std::string(char_array, size);
}

json get_json_contents(const std::string& path)
{
    // returns contents from json file
    std::ifstream file(path);
    if(!file.is_open())
    {
        throw std::runtime_error("[UTILS] Could not read json file! Please check system permissions and try again!");
    }

    // reads json contents
    json data;
    file >> data;

    file.close();

    return data;
}

void save_json_to_file(json data, const std::string& filename) 
{
    std::ofstream file(filename);
    if (file.is_open()) 
    {
        file << data;
        file.close();
    } 
    else 
    {
        throw std::runtime_error("[UTILS] Could not open file '" + filename + "'!");
    }
}

std::string get_machine_name()
{
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != -1) 
    {
        return hostname;
    } else 
    {
        return "UNKNOWN";
    }
    return "";
}

std::time_t get_time()
{
    std::time_t current_time;
    std::time(&current_time);
    return current_time;
}

std::string get_file_name(std::string& path)
{
    fs::path path_obj(path);
    return path_obj.filename().string();
}

std::string calculate_md5_checksum(const std::string& filepath) 
{
    CryptoPP::Weak1::MD5 md5;
    std::string digest;

    CryptoPP::FileSource file(filepath.c_str(), true,
        new CryptoPP::HashFilter(md5, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));

    return digest;
}

std::vector<std::vector<char>> bufferize_file(std::string file_path, std::size_t buffer_size)
{
    std::ifstream file(file_path, std::ios::binary);
    
    if(!file.is_open()) 
    {
        throw std::runtime_error("[UTILS] Could not bufferize file \"" + file_path + "\"");
    }

    std::vector<std::vector<char>> buffers;

    // bufferize file
    while(!file.eof()) 
    {
        std::vector<char> buffer(buffer_size);
        file.read(buffer.data(), buffer_size);
        std::streamsize bytes_read = file.gcount();

        if(bytes_read > 0) 
        {
            buffer.resize(static_cast<std::size_t>(bytes_read));
            buffers.push_back(buffer);
        }
    }

    file.close();
    return buffers;
}

void rename_replacing(std::string& old_path, std::string& new_path)
{
    if(!std::rename(old_path.c_str(), new_path.c_str()))
    {
        throw std::runtime_error("[UTILS] Could nome rename file " + old_path);
    }
}

std::string file_without_extension(std::string file_name) 
{
    size_t last_dot_pos = file_name.find_last_of('.');
    if (last_dot_pos != std::string::npos) 
    {
        return file_name.substr(0, last_dot_pos);
    }
    return file_name;
}

int get_folder_space(std::string folder_path, std::string param = "")
{
    if (fs::exists(folder_path)) 
    {
        if(fs::is_directory(folder_path))
        {
            fs::space_info space = fs::space(folder_path);
            if(param == "free")
            {
                return static_cast<int>(space.free);
            }
            else if(param == "used")
            {
                return static_cast<int>(space.capacity - space.free);

            }
            else if(param == "total")
            {
                return static_cast<int>(space.capacity);
            }
            else
            {
                throw std::runtime_error("[UTILS] No valid parameter specified!");
            }

        }
        else
        {
            throw std::runtime_error("[UTILS] Given path is not a valid folder!");
        }
    } 
    else 
    {
        throw std::runtime_error("[UTILS] Given path does not exist!");
    }
}

std::vector<std::string> split_buffer(const char* buffer) 
{
    std::vector<std::string> tokens;
    
    const char* start_pos = buffer;
    const char* end_pos = std::strchr(buffer, '|');
    
    while(end_pos != nullptr) 
    {
        tokens.push_back(std::string(start_pos, end_pos - start_pos));
        start_pos = end_pos + 1;
        end_pos = std::strchr(start_pos, '|');
    }
    tokens.push_back(std::string(start_pos));
    return tokens;
}