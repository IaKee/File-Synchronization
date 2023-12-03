// c++
#include <iostream>
#include <csignal>
#include <stdexcept>
#include <exception>

// standard c
#include <termios.h>
#include <unistd.h>

// locals
#include "../../common/include/external/cxxopts.hpp"
#include "../../common/include/asyncio/async_cout.hpp"
#include "../../common/include/utils.hpp"
#include "application.hpp"

using namespace async_cout;

struct termios old_settings, new_settings;
void cleanup(int signal)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
	std::cout << "\n\n[CLEANUP] Restored terminal to cooked mode..." << std::endl;

	stop_capture();
	std::exit(signal);
}

int main(int argc, char* argv[]) 
{
    // saves current terminal mode
	std::cout << "[MAIN][CLEANUP] Saving terminal current mode..." << std::endl;
    tcgetattr(STDIN_FILENO, &old_settings);
	std::cout << "[MAIN][CLEANUP] Setting terminal to raw mode..." << std::endl;
    new_settings = old_settings;
    new_settings.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
	std::signal(SIGINT, cleanup);
	start_capture();

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
    const std::string SERVER_IP_DESCRIPTION = "Use this option to specify the IP address of the \
    SyncWizard server you want to use. This will allow the program to connect to the correct \
    server for file synchronization.";
    const std::string SERVER_PORT_DESCRIPTION = "Use this option to specify the port number of \
    the SyncWizard server you want to use. The program will use this port to establish a \
    connection with the server.";
    const std::string HELP_DESCRIPTION = "This option displays the description of the available \
    program arguments. If you need help or have questions about how to use SyncWizard, you \
    can use this option to get more information.";
    const std::string RUN_INFO = "Please run './SyncWizard -h' for more information.";
    const std::string USERNAME_DESCRIPTION = "Use this option to specify the username to which \
    the files will be associated. This is important for identifying the synchronized files \
    belonging to each user. It must be between 1 and 12 characters long, without spaces or \
    symbols.";
    
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
            std::cout << "\t[MAIN] " << options.help() << std::endl;
            cleanup(0);
            return 0;
        }

        // verifying if all required options were provided
        if (result.count("username") == 0 || result.count("server_ip") == 0 || result.count("port") == 0)
        {
            // even not being parsed by using cxxopts expected layout, arguments may still be there
            // tries to parse, and later validates them
            
            if(argc < 4)
            {
                std::cout << "\t[MAIN] " + ERROR_PARSING_MISSING << std::endl;
                std::cout << "\t[MAIN] " + CLIENT_USAGE_LAYOUT << std::endl;
                std::cout << "\t[MAIN] " + RUN_INFO << std::endl;

                cleanup(0);
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
        std::cerr << "\t[MAIN] " + ERROR_PARSING_CRITICAL << e.what() << std::endl;
        cleanup(0);
        return -1;
    }
    
    try
    {
        client_application::Client app(username, server_ip, port);
        app.start();
    }
    catch(const std::exception& e)
    {
        std::cerr << "\n[MAIN] Critical error:\n\t\t" << e.what() << std::endl;
        cleanup(0);
        return -1;
    }
    
    cleanup(0);
    return 0;
}