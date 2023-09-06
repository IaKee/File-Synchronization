// c++
#include <iostream>
#include <exception>
#include <stdexcept>
#include <string>
#include <tuple>
#include <cstring>

// multithreading & synchronization
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>

// argument parsing
#include "../include/common/cxxopts.hpp"

// local
#include "client_app.hpp"
#include "../include/common/user_interface.hpp"
#include "../include/common/connection_manager.hpp"
#include "../include/common/utils.hpp"
#include "../include/common/inotify_watcher.hpp"

using namespace client_app;

Client::Client(
    std::string username, 
    std::string server_address, 
    int server_port)
    :   username_(username),  
        UI_(
            &ui_buffer_mtx_, 
            &ui_cv_, 
            &ui_buffer_, 
            &ui_sanitized_buffer_),
        connection_manager_()
{   
    // initialization sequence
    machine_name_ = get_machine_name();

    // lang strings
    async_utils::async_print("[STARTUP] SyncWizard Client initializing on " + machine_name_ + "...");
    std::string ERROR_PARSING_CRITICAL = "Critical error parsing command-line options:";
    std::string ERROR_PARSING_MISSING = "Missing required command-line options!";
    std::string ERROR_PARSING_USERNAME = "Invalid username! It must be between 1 and 12 characters long, \
    without spaces or symbols.";
    std::string ERROR_PARSING_IP = "Invalid IP address! The correct format should be XXX.XXX.XXX.XXX, \
    where X represents a number from 0 to 255, and there should be dots (.) separating the sections.";
    std::string ERROR_PARSING_PORT = "Invalid port! The correct port should be between 0 and 65535." ;
    const std::string HELP_DESCRIPTION = "This option displays the description of the available \
    program arguments. If you need help or have questions about how to use SyncWizard, you \
    can use this option to get more information.";
    const std::string RUN_INFO = "Please run './SyncWizard -h' for more information.";
    const std::string USERNAME_DESCRIPTION = "Use this option to specify the username to which \
    the files will be associated. This is important for identifying the synchronized files \
    belonging to each user. It must be between 1 and 12 characters long, without spaces or \
    symbols.";
    const std::string EXIT_MESSAGE = "Exiting the program...";
    
    // validates launch arguments
    std::string error_description = "";
    if(!is_valid_username(username_))
    {
        error_description = ERROR_PARSING_USERNAME;
    }
    else if(!is_valid_address(server_address))
    {
        error_description = ERROR_PARSING_IP;
    }
    else if(!is_valid_port(server_port))
    {
        error_description = ERROR_PARSING_PORT;
    }    

    // stops initialization if some argument is not valid
    if(error_description != "")
    {   
        async_utils::async_print("\t[SYNCWIZARD CLIENT] " + error_description);
        async_utils::async_print("\t[SYNCWIZARD CLIENT] " + RUN_INFO);
        throw std::invalid_argument(ERROR_PARSING_CRITICAL);
    }

    try
    {
        // creates socket
        async_utils::async_print("\t[SYNCWIZARD CLIENT] Creating socket...");
        connection_manager_.create_socket();

        // resolves host name
        async_utils::async_print("\t[SYNCWIZARD CLIENT] Resolving host name...");
        std::string server_adjusted_address = connection_manager_.get_host_by_name(server_address);

        // tries to connect to server
        async_utils::async_print("\t[SYNCWIZARD CLIENT] Attempting connection to server...");
        connection_manager_.connect_to_server(server_address, server_port);
        
        // after being connected tries to send login request
        session_id_ = connection_manager_.login(username_, machine_name_);

        // sets running flag to true
        running_app_.store(true);

        // initializes user interface last
        UI_.start();
    }
    catch(const std::exception& e)
    {   
        // propagates error
        throw std::runtime_error("[CLIENT APP] Critical error intializing:\n\t\t" + std::string(e.what()));
    }
    
};

Client::~Client()
{
    // on destroy
    // TODO: kill child threads here
    // TODO: close connection here
    close();
};

      
bool Client::set_sync_dir_(std::string new_directory) 
{
    // ensures a valid directory as syncDir
    if(!is_valid_path(new_directory))
    {
       if(!create_directory(new_directory))
       {
            throw std::runtime_error("[CLIENT APP] Could not acess informed sync dir path!");
       }
       return true;  // sync dir created
    }
    return false;  // sync dir is valid and acessible
}

void Client::main_loop()
{
    try
    {
        while(running_app_.load())
        {  
            {
                std::unique_lock<std::mutex> lock(ui_buffer_mtx_);
                ui_cv_.wait(
                    lock, 
                    [this]
                    {
                        return (ui_buffer_.size() > 0 || running_app_.load() == false);
                    });
            }

            // checks again if the client should be running
            if(running_app_.load() == false)
            {
                ui_cv_.notify_one();
                break;
            }

            process_user_interface_commands_();

            // resets shared buffers
            ui_buffer_.clear();
            ui_sanitized_buffer_.clear();

            // notifies UI_ that the buffer is free to be used again
            ui_cv_.notify_one();
        }
    }
    catch (const std::exception& e)
    {
        async_utils::async_print("[SYNCWIZARD CLIENT] Exception occured on main_loop():\n\t\t" + std::string(e.what()));
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
        async_utils::async_print("[SYNCWIZARD CLIENT] Exception occured on start():\n\t\t" + std::string(e.what()));
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
        async_utils::async_print("\t[SYNCWIZARD CLIENT] Exiting the program...");

        running_app_.store(false);
        UI_.stop();
        ui_cv_.notify_all();

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