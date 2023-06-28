#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <pthread.h>
#include <semaphore.h>
#include <boost/asio.hpp>

#include "client_commands.hpp"
//#include "../include/common/packet.h"
#include "../include/common/cxxopts.hpp"
#include "../include/common/constants.hpp"
#include "../include/common/utils.hpp"
#include "../include/common/command_reader.hpp"


class Client
{
    private:
        std::string sync_dir;
        std::string username;
        std::string server_address;

    public:
        // simple setters
        void set_username(std::string new_username){username = new_username;}
        
        // setters with some treatment
        void set_sync_dir(std::string new_directory)
        {
            // TODO: if invlaid, ask for new path, or show suggestion

            // ensures a valid directory as syncDir
            if(!is_valid_path(new_directory))
                throw std::runtime_error("invalidpath");
        }
        
        // getters
        std::string get_username(){return username;}

        // other
        

};

int main(int argc, char* argv[]) 
{
    // TODO: parse arguments
    // TODO: print sync dir
    // TODO: look for further commands
    // TODO: error treatment
    // TODO: interface
    // TODO: sistema de username

    // main attributes
    std::string username;
    std::string server_ip;
    int port;
    bool show_help = false;  // defaults false

    // expected program arguments definition
    cxxopts::Options options(PROGRAM_NAME, PROGRAM_DESCRIPTION);
    options.add_options()
        ("u,username", USERNAME_DESCRIPTION, cxxopts::value<std::string>(username))
        ("s,server_ip", SERVER_IP_DESCRIPTION, cxxopts::value<std::string>(server_ip))
        ("p,port", SERVER_PORT_DESCRIPTION, cxxopts::value<int>(port))
        ("h,help", HELP_DESCRIPTION, cxxopts::value<bool>(show_help))
        ("e, example", EXAMPLE_USAGE);

    // parses and verifies recieved program arguments
    cxxopts::ParseResult result;
    try 
    {
        auto result = options.parse(argc, argv);

        
        // help command-line option
        if (show_help) 
        {
            std::cout << options.help() << std::endl;
            return 0;
        }

        // verifying if all required options were provided
        if (result.count("username") == 0 || result.count("server_ip") == 0 || result.count("port") == 0) 
        {
            // even not being parsed by using cxxopts expected layout, arguments may still be there
            // tries to parse, and later validates them
            
            if(argc < 4)
            {
                std::cout << MISSING_COMMAND_OPTIONS << std::endl;
                std::cout << MAIN_USAGE << std::endl;
                std::cout << RUN_INFO << std::endl;
                return -1;
            }

            username = argv[1];
            server_ip = argv[2];
            port = std::stoi(argv[3]);
        }

        // command line validation
        if(!is_valid_username(username))
        {
            std::cout << USERNAME_ERROR << std::endl;
            std::cout << RUN_INFO << std::endl;
            return -1;
        }
        if(!is_valid_IP(server_ip))
        {
            std::cout << IP_ERROR << std::endl;
            std::cout << RUN_INFO << std::endl;
            return -1;
        }
        if(!is_valid_port(port))
        {
            std::cout << PORT_ERROR << std::endl;
            std::cout << RUN_INFO << std::endl;
            return -1;
        }
    } catch (std::exception& e) 
    {
        std::cerr << ERROR_PARSING << e.what() << std::endl;
        return -1;
    }

    // starts async command line reader - giving appropriate command callbacks
    try
    {
        boost::asio::io_context async_io_context;

        command_reader::CommandReader user_interface(async_io_context, commands);

        insert_prefix();

        user_interface.start();
        async_io_context.run();

        user_interface.stop();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    return 0;
}

