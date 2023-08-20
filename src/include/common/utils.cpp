// c++
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <regex>
#include <string>
#include <unistd.h>
#include <filesystem>
#include <fstream>

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
    int portValue = port;
    return portValue >= 0 && portValue <= 65535;
}

bool is_valid_path(std::string path)
{
    // checks if the informed folder does exist within the file system
    return (std::filesystem::exists(path)) ? true : false;
}

bool create_directory(const std::string& path) 
{
    return std::filesystem::create_directory(path);
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

json get_json_contents(const std::string& path)
{
    // returns contents from json file
    std::ifstream file(path);
    if(!file.is_open())
    {
        throw std::runtime_error("Could not read json file! Please check system permissions and try again!");
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
        throw std::runtime_error("Could not open file '" + filename + "'!");
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
}

std::time_t get_time()
{
    std::time_t current_time;
    std::time(&current_time);
    return current_time;
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
                throw std::runtime_error("No valid parameter specified!");
            }

        }
        else
        {
            throw std::runtime_error("Given path is not a valid folder!");
        }
    } 
    else 
    {
        throw std::runtime_error("Given path does not exist!");
    }
}