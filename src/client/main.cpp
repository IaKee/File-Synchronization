// c++
#include <iostream>

// locals
#include "../include/common/cxxopts.hpp"
#include "client_app.hpp"

int main(int argc, char* argv[]) 
{
    // main attributes
    std::string username;
    std::string server_ip;
    int port;
    bool show_help = false;  // defaults false

    // lang strings
    std::string CLIENT_PROGRAM_NAME = "SyncWizard Client";
    const std::string CLIENT_PROGRAM_DESCRIPTION = "SyncWizard client is a file synchronization program. \
    It allows you to synchronize your files on a remote server for access and sharing. \
    All non-help command-line arguments are required to run. Below are the details \
    about each one:";
    std::string CLIENT_USAGE_LAYOUT = "Usage: ./SWizClient -u <username> -s <server_ip_address> -p <port>";
    std::string CLIENT_ALT_USAGE_LAYOUT = "Alternative usage: ./SWizClient <username> <server_ip_address> <port>";
    std::string CLIENT_EXAMPLE_USAGE = "Example usage: ./SWizClient --username john \
    --server_ip 192.168.0.100 --port 8080";
    std::string ERROR_PARSING_CRITICAL = "Critical error parsing command-line options:";
    std::string ERROR_PARSING_MISSING = "Missing required command-line options!";
    std::string ERROR_PARSING_USERNAME = "Invalid username! It must be between 1 and 12 characters long, \
    without spaces or symbols.";
    std::string ERROR_PARSING_IP = "Invalid IP address! The correct format should be XXX.XXX.XXX.XXX, \
    where X represents a number from 0 to 255, and there should be dots (.) separating the sections.";
    std::string ERROR_PARSING_PORT = "Invalid port! The correct port should be between 0 and 65535." ;

    // expected program arguments - definition
    cxxopts::Options options(CLIENT_PROGRAM_NAME, CLIENT_PROGRAM_DESCRIPTION);
    options.add_options()
        ("u,username", USERNAME_DESCRIPTION, cxxopts::value<std::string>(username))
        ("s,server_ip", SERVER_IP_DESCRIPTION, cxxopts::value<std::string>(server_ip))
        ("p,port", SERVER_PORT_DESCRIPTION, cxxopts::value<int>(port))
        ("h,help", HELP_DESCRIPTION, cxxopts::value<bool>(show_help))
        ("e, example", CLIENT_EXAMPLE_USAGE);

    // parses and verifies recieved program arguments
    cxxopts::ParseResult result;
    try
    {
        auto result = options.parse(argc, argv);

        
        // help command-line option
        if(show_help)
        {
            std::cout << "\t" << options.help() << std::endl;
            return 0;
        }

        // verifying if all required options were provided
        if (result.count("username") == 0 || result.count("server_ip") == 0 || result.count("port") == 0)
        {
            // even not being parsed by using cxxopts expected layout, arguments may still be there
            // tries to parse, and later validates them
            
            if(argc < 4)
            {
                std::cout << "\t[CLIENT] " + ERROR_PARSING_MISSING << std::endl;
                std::cout << "\t[CLIENT] " + CLIENT_USAGE_LAYOUT << std::endl;
                std::cout << "\t[CLIENT] " + RUN_INFO << std::endl;
                return -1;
            }

            username = argv[1];
            server_ip = argv[2];
            port = std::stoi(argv[3]);
        } 
        else 
        {
            // arguments provided
            username = result["username"].as<std::string>();
            server_ip = result["server_ip"].as<std::string>();
            port = result["port"].as<int>();
        }
    } 
    catch (std::exception& e) 
    {
        std::cerr << "\t[CLIENT] " + ERROR_PARSING_CRITICAL << e.what() << std::endl;
        return -1;
    }
    
    try
    {
        client_app::Client app(username, server_ip, port);
        app.start();
    }
    catch(const std::exception& e)
    {
        std::cerr << "\nException captured on main(): " << e.what() << std::endl;
    }
    

    return 0;
}