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
        // runtime control
        bool running_;
        std::atomic<bool> stop_requested_;

        // connection
        std::string username_;
        std::string server_address_;
        connection::Connection connection_;
        int server_port_;

        // file management
        std::string sync_dir_;

        // input
        user_interface::UserInterface UI_; 

    public:
        // on init
        Client(const std::string& u, const std::string& add, const int& p)
            :   username_(u), 
                server_address_(add), 
                server_port_(p), 
                UI_(mutex_, cv_, command_buffer_, sanitized_commands_),
                connection_(),
                running_(true),
                stop_requested_(false)
        {
            // initialization sequence
            std::string machine_name = get_machine_name();
            std::cout << "[STARTUP] " << CLIENT_PROGRAM_NAME << " " << CLIENT_PROGRAM_VERSION <<
                " initializing on " << machine_name << std::endl;

            
            // validates launch arguments
            std::string error_description = "";
            if(!is_valid_username(username_))
                error_description = ERROR_PARSING_USERNAME;
            if(!is_valid_IP(server_address_))
                error_description = ERROR_PARSING_IP;
            if(!is_valid_port(server_port_))
                error_description = ERROR_PARSING_PORT;

            if(error_description != "")
            {   
                std::cout << PROMPT_PREFIX << error_description << std::endl;
                std::cout << PROMPT_PREFIX << RUN_INFO << std::endl;
                throw std::invalid_argument(ERROR_PARSING_CRITICAL);
            }

            // creates socket
            std::cout << PROMPT_PREFIX_CLIENT << SOCK_CREATING;
            bool sock_ok = connection_.create_socket();
            if(!sock_ok)
            {
                running_ = false;
                throw std::runtime_error(ERROR_SOCK_CREATING);
            }
            std::cout << DONE_SUFIX << std::endl;


            // resolves host name
            server_address_ = connection_.get_host_by_name(add);
            std::cout << DONE_SUFIX << std::endl;

            // connects to server
            //std::cout << PROMPT_PREFIX_CLIENT << CONNECTING_TO_SERVER;
            //std::cout.flush();
            //connection_.connect_to_server(server_address_, server_port_);
            //std::cout << DONE_SUFIX << std::endl;

            // initializes user interface last
            std::cout << PROMPT_PREFIX_CLIENT << UI_STARTING;
            UI_.start();
            std::cout << DONE_SUFIX << std::endl;

        };

        // on destroy
        ~Client()
        {

            // TODO: kill child threads here
            // TODO: close connection here
            close();
        }

        // attributes
        std::string command_buffer;
        //std::list<std::thread> thread_list;

        // synchronization
        std::mutex mutex_;
        std::condition_variable cv_;
        std::string command_buffer_;
        std::list<std::string> sanitized_commands_;

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
                        stop();
                    }
                    else
                        std::cout << PROMPT_PREFIX_CLIENT << ERROR_COMMAND_INVALID << std::endl;

                    break;
                case 2:
                    break;
                
                default:
                    std::cout << PROMPT_PREFIX_CLIENT << ERROR_COMMAND_INVALID << std::endl;
                    break;
            }
        }

        void main_loop()
        {
            try
            {
                while(running_)
                {  
                    {
                        // inserts promt prefix before que actual user command
                        std::lock_guard<std::mutex> lock(mutex_);
                        std::cout << PROMPT_PREFIX;
                        std::cout.flush();
                    }   

                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        cv_.wait(lock, [this]{return command_buffer_.size() > 0 || stop_requested_.load();});
                    }

                    // checks again if the client should be running
                    if(stop_requested_.load())
                    {
                        cv_.notify_one();
                        break;
                    }

                    process_input();

                    // resets shared buffers
                    command_buffer_ = "";
                    sanitized_commands_.clear();

                    // notifies UI_ that the buffer is free to be used again
                    cv_.notify_one();
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "[ERROR][CLIENT] Error occured on main_loop(): " << e.what() << std::endl;
            }
            catch (...) 
            {
                std::cerr << "[ERROR][CLIENT] Unknown error occured on main_loop()!" << std::endl;
            }
        }

        void start()
        {
            try
            {
                main_loop();
            }
            catch(const std::exception& e)
            {
                std::cerr << "[ERROR][CLIENT] Error occured on start(): " << e.what() << std::endl;
                throw std::runtime_error(e.what());
            }
            catch(...)
            {
                std::cerr << "[ERROR][CLIENT] Unknown error occured on start()!" << std::endl;
                throw std::runtime_error("Unknown error occured on start()!");
            }
            
        }

        void stop()
        {
            // stop user interface and command processing
            try
            {
                std::cout << PROMPT_PREFIX_CLIENT << EXIT_MESSAGE << std::endl;

                stop_requested_.store(true);
                running_ = false;
                
                UI_.stop();
                
                cv_.notify_all();
    
            }
            catch(const std::exception& e)
            {
                std::cerr << "[ERROR][CLIENT] Error occured on stop(): " << e.what() << std::endl;
                throw std::runtime_error(e.what());
            }
            catch(...)
            {
                std::cerr << "[ERROR][CLIENT] Unknown error occured on stop()!" << std::endl;
                throw std::runtime_error("Unknown error occured on stop()!");
            }
        }

        void close()
        {
            // closes remaining threads
            UI_.input_thread_.join();
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
    
    try
    {
        Client app(username, server_ip, port);
        app.start();
    }
    catch(const std::exception& e)
    {
        std::cerr << "\nException captured on main(): " << e.what() << std::endl;
    }
    

    return 0;
}

