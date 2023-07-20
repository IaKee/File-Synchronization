
#include <iostream>
#include <regex>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <filesystem>

#include "utils.hpp"

void insert_prefix()
{
    /* Inserts a prefix on the terminal to differentiate when the program is
    actually running from the system default*/
    std::cout << "\t#> ";
    std::cout.flush();
}

bool is_valid_IP(const std::string& ip_address) 
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
