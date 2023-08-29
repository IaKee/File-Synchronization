// c++
#include <iostream>
#include <exception>
#include <stdexcept>
#include <string>

// multithreading & synchronization
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>

// argument parsing

// local
#include "client_app.hpp"
#include "../include/common/cxxopts.hpp"
#include "../include/common/user_interface.hpp"
#include "../include/common/connection.hpp"
#include "../include/common/utils.hpp"
#include "../include/common/inotify_watcher.hpp"

using namespace client_app;

Client::Client(const std::string& u, const std::string& add, const int& p)
    :   username_(u), 
        server_address_(add), 
        server_port_(p), 
        UI_(mutex_, cv_, command_buffer_, sanitized_commands_),
        connection_(),
        running_(true),
        stop_requested_(false),
        inotify_()
{
    // initialization sequence
    std::string machine_name = get_machine_name();

    // lang strings
    async_utils::async_print("[STARTUP] SyncWizard Client initializing on " + machine_name + "...");
    std::string ERROR_PARSING_CRITICAL = "Critical error parsing command-line options:";
    std::string ERROR_PARSING_MISSING = "Missing required command-line options!";
    std::string ERROR_PARSING_USERNAME = "Invalid username! It must be between 1 and 12 characters long, \
    without spaces or symbols.";
    std::string ERROR_PARSING_IP = "Invalid IP address! The correct format should be XXX.XXX.XXX.XXX, \
    where X represents a number from 0 to 255, and there should be dots (.) separating the sections.";
    std::string ERROR_PARSING_PORT = "Invalid port! The correct port should be between 0 and 65535." ;
    
    // validates launch arguments
    std::string error_description = "";
    if(!is_valid_username(username_))
        error_description = ERROR_PARSING_USERNAME;
    if(!is_valid_address(server_address_))
        error_description = ERROR_PARSING_IP;
    if(!is_valid_port(server_port_))
        error_description = ERROR_PARSING_PORT;

    if(error_description != "")
    {   
        async_utils::async_print("\t[SYNCWIZARD CLIENT] " + error_description);
        async_utils::async_print("\t[SYNCWIZARD CLIENT] " + RUN_INFO);
        throw std::invalid_argument(ERROR_PARSING_CRITICAL);
    }

    // creates socket
    connection_.create_socket();
    
    // resolves host name
    server_address_ = connection_.get_host_by_name(add);

    // connects to server
    async_utils::async_print("\t[SYNCWIZARD CLIENT] " + CONNECTING_TO_SERVER);
    connection_.connect_to_server(server_address_, server_port_);
    
    std::string login_string = username_ + "|" + machine_name;
    char buffer[1024];
    strcpy(buffer, login_string.c_str());
    connection_.send_data(buffer, sizeof(buffer));
    
    // initializes user interface last
    UI_.start();
};

Client::~Client()
{
    // on destroy
    // TODO: kill child threads here
    // TODO: close connection here
    close();
};

      
void Client::set_sync_dir(std::string new_directory) 
{
    // TODO: if invalid, ask for new path, or show suggestion

    // ensures a valid directory as syncDir
    if(!is_valid_path(new_directory))
    {
        running_ = false;
        throw std::runtime_error("[SYNCWIZARD CLIENT] Given path is invalid!");
    }
}

void Client::process_input()
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
                async_utils::async_print("\t[SYNCWIZARD CLIENT] Stop requested.");
                stop();
                break;
            }
            async_utils::async_print("\t[SYNCWIZARD CLIENT] Could not find a valid command by \"" + command_buffer_ + "\"!");
            break;
        case 2:
            if(sanitized_commands_[0] == "get" && sanitized_commands_[1] == "sync_dir")
            {
                //if(is_valid_path())
                //create_directory()
                break;
            }
            else
            {
                async_utils::async_print("\t[SYNCWIZARD CLIENT] Could not find a valid command by \"" + command_buffer_ + "\"!");
                break;
            }
            break;
        case 3:
            if(sanitized_commands_[0] == "get" && sanitized_commands_[1] == "sync" && sanitized_commands_[2] == "dir")
            {
                break;
            }
            else
            {
                async_utils::async_print("\t[SYNCWIZARD CLIENT] Could not find a valid command by \"" + command_buffer_ + "\"!");
                break;
            }
        default:
            async_utils::async_print("\t[SYNCWIZARD CLIENT] Could not find a valid command by \"" + command_buffer_ + "\"!");
            break;
    }
}

void Client::main_loop()
{
    try
    {
        while(running_)
        {  
            {
                // inserts promt prefix before que actual user command
                std::lock_guard<std::mutex> lock(mutex_);
                //std::cout << "\t#> ";  // prompt prefix
                //std::cout.flush();
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
            command_buffer_.clear();
            sanitized_commands_.clear();

            // notifies UI_ that the buffer is free to be used again
            cv_.notify_one();
        }
    }
    catch (const std::exception& e)
    {
        async_utils::async_print("[SYNCWIZARD CLIENT] Exception occured on main_loop(): " + std::string(e.what()));
        throw std::runtime_error(e.what());
    }
    catch (...) 
    {
        async_utils::async_print("[SYNCWIZARD CLIENT] Unknown exception occured on main_loop()!");
        throw std::runtime_error("Unknown error occured on main_loop()!");
    }
}

void Client::start()
{
    try
    {
        main_loop();
    }
    catch(const std::exception& e)
    {
        async_utils::async_print("[SYNCWIZARD CLIENT] Exception occured on start(): " + std::string(e.what()));
        throw std::runtime_error(e.what());
    }
    catch(...)
    {
        async_utils::async_print("[SYNCWIZARD CLIENT] Unknown exception occured on start()!");
        throw std::runtime_error("Unknown error occured on start()!");
    }
    
}

void Client::stop()
{
    // stop user interface and command processing
    try
    {
        async_utils::async_print("\t[SYNCWIZARD CLIENT] " + EXIT_MESSAGE);

        stop_requested_.store(true);
        running_ = false;
        
        UI_.stop();
        
        cv_.notify_all();

    }
    catch(const std::exception& e)
    {
        async_utils::async_print("[SYNCWIZARD CLIENT] Error occured on stop(): " + std::string(e.what()));
        throw std::runtime_error(e.what());
    }
    catch(...)
    {
        async_utils::async_print("[SYNCWIZARD CLIENT] Unknown error occured on stop()!");
        throw std::runtime_error("Unknown error occured on stop()!");
    }
}

void Client::close()
{
    // closes remaining threads
    UI_.input_thread_.join();
}

