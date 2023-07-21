// c++
#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <atomic>
#include <cstdlib>

// multithreading
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>


// argument parsing
#include "../include/common/cxxopts.hpp"

// local
#include "client_commands.hpp"
#include "../include/common/lang.hpp"
#include "../include/common/user_interface.hpp"
#include "../include/common/connection.hpp"
#include "../include/common/utils.hpp"


// namespace
using cxxopts::Options;
using cxxopts::ParseResult;

class Client
{
    private:
        // connection
        std::string username_;
        std::string server_address_;
        int server_port_;
        connection::Connection connection_;

        // file management
        std::string sync_dir_;

        // input
        user_interface::UserInterface UI_; 

        // synchronization
        std::mutex mutex_;
        std::condition_variable buffer_status_;
        std::condition_variable collector_status_;
        std::string command_buffer_;
        std::list<std::string> sanitized_commands_;

        // program flow
        bool running_ = true;

        bool validate_arguments_()
        {
            std::string error_description = "";
            
            if(!is_valid_username(this->username_))
                error_description = ERROR_PARSING_USERNAME;
            if(!is_valid_IP(this->server_address_))
                error_description = ERROR_PARSING_IP;
            if(!is_valid_port(this->server_port_))
                error_description = ERROR_PARSING_PORT;

            if(error_description != "")
            {   
                std::cout << PROMPT_PREFIX << error_description << std::endl;
                std::cout << PROMPT_PREFIX << RUN_INFO << std::endl;
                running_ = false;
                return false;
            }
            return true;
        }

    public:
        // on init
        Client(const string& u, const string& add, const int& p)
            :   username_(u), 
                server_address_(add), 
                server_port_(p), 
                UI_(mutex_, buffer_status_, collector_status_, command_buffer_, sanitized_commands_),
                connection_(),
                running_(true)
        {
            // initialization sequence
            std::string machine_name = get_machine_name();
            std::cout << CLIENT_PROGRAM_NAME << " " << CLIENT_PROGRAM_VERSION <<
                " started at " << machine_name << std::endl;

            bool arg_ok = validate_arguments_();  // user, server address and port
            
            if(!arg_ok)
            {
                throw std::invalid_argument(ERROR_PARSING_CRITICAL);
            }

            // creates socket
            bool sock_ok = connection_.create_socket();
            if(!sock_ok)
            {
                running_ = false;
                throw std::runtime_error(ERROR_SOCK_CREATING);
            }

            // resolves host name
            server_address_ = connection_.get_host_by_name(add);

            // connects to server
            connection_.connect_to_server(server_address_, server_port_);

            // initializes user interface last
            UI_.start();
        };

        // on destroy
        ~Client()
        {
            // TODO: kill child threads here
            // TODO: close connection here
            //quit();
        }

        // attributes
        std::string command_buffer;
        std::list<std::thread> thread_list;

        void set_sync_dir(std::string new_directory) 
        {
            // TODO: if invalid, ask for new path, or show suggestion

            // ensures a valid directory as syncDir
            if(!is_valid_path(new_directory))
            {
                running_ = false;
                throw std::runtime_error(ERROR_PATH_INVALID);
            }

        }

        void process_input()
        {
            // process input buffer and calls methods accordingly
            int nargs = sanitized_commands_.size();

            switch (nargs)
            {
                case 0:
                    break;
                case 1:
                    if(sanitized_commands_.front() == "exit")
                    {
                        quit();
                    }
                    else
                        std::cout << PROMPT_PREFIX_CLIENT << ERROR_COMMAND_INVALID << std::endl;

                    break;
                case 2:
                    break;
                
                default:
                    //std::cout << PROMPT_PREFIX << ERROR_COMMAND_USAGE << sanitized_commands_.front() << std::endl;
                    std::cout << PROMPT_PREFIX_CLIENT << ERROR_COMMAND_INVALID << std::endl;
                    break;
            }
        }

        void main_loop()
        {
            while(running_)
            {   
                // inserts promt prefix before que actual user command
                std::cout << PROMPT_PREFIX;
                std::cout.flush();

                // waits for the buffer to actually have something
                std::unique_lock<std::mutex> lock(mutex_);

                std::cout << "consumer: about to wait" << std::endl;
                collector_status_.wait(lock, [this]() 
                { 
                    return command_buffer_.size() > 0 || !running_; 
                });


                process_input();

                // resets shared buffers
                command_buffer_ = "";
                sanitized_commands_.clear();
                std::cout << "hello from consumer!" << std::endl;

                // notifies UI_ that the buffer is free to be used again
                buffer_status_.notify_one();
            }

        }

        void quit()
        {
            // closing sequence
            std::cout << PROMPT_PREFIX_CLIENT << EXIT_MESSAGE << std::endl;
            
            running_ = false;
            buffer_status_.notify_all();
            UI_.stop();

            command_buffer_ = "";
            sanitized_commands_.clear();
            buffer_status_.notify_all();

            //exit(0);
        }
};

int main(int argc, char* argv[]) 
{
    // main attributes
    std::string username;
    std::string server_ip;
    int port;
    bool show_help = false;  // defaults false

    // expected program arguments - definition
    Options options(CLIENT_PROGRAM_NAME, CLIENT_PROGRAM_DESCRIPTION);
    options.add_options()
        ("u,username", USERNAME_DESCRIPTION, cxxopts::value<std::string>(username))
        ("s,server_ip", SERVER_IP_DESCRIPTION, cxxopts::value<std::string>(server_ip))
        ("p,port", SERVER_PORT_DESCRIPTION, cxxopts::value<int>(port))
        ("h,help", HELP_DESCRIPTION, cxxopts::value<bool>(show_help))
        ("e, example", CLIENT_EXAMPLE_USAGE);

    // parses and verifies recieved program arguments
    ParseResult result;
    try
    {
        auto result = options.parse(argc, argv);

        
        // help command-line option
        if(show_help)
        {
            std::cout << PROMPT_PREFIX << options.help() << std::endl;
            return 0;
        }

        // verifying if all required options were provided
        if (result.count("username") == 0 || result.count("server_ip") == 0 || result.count("port") == 0)
        {
            // even not being parsed by using cxxopts expected layout, arguments may still be there
            // tries to parse, and later validates them
            
            if(argc < 4)
            {
                std::cout << PROMPT_PREFIX << ERROR_PARSING_MISSING << std::endl;
                std::cout << PROMPT_PREFIX << CLIENT_USAGE_LAYOUT << std::endl;
                std::cout << PROMPT_PREFIX << RUN_INFO << std::endl;
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
        std::cerr << PROMPT_PREFIX << ERROR_PARSING_CRITICAL << e.what() << std::endl;
        return -1;
    }
    
    Client app(username, server_ip, port);
    app.main_loop();

    return 0;
}

