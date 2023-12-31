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
#include "../include/common/async_cout.hpp"
#include "../include/common/utils.hpp"
#include "../include/common/inotify_watcher.hpp"

using namespace client_app;
using namespace async_cout;

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
    aprint("Initializing on " + machine_name_ + "...");
    
    std::string ERROR_PARSING_MISSING = "Missing required command-line options!";
    std::string HELP_DESCRIPTION = "This option displays the description of the available \
    program arguments. If you need help or have questions about how to use SyncWizard, you \
    can use this option to get more information.";
    std::string RUN_INFO = "Please run './SyncWizard -h' for more information.";
    std::string USERNAME_DESCRIPTION = "Use this option to specify the username to which \
    the files will be associated. This is important for identifying the synchronized files \
    belonging to each user. It must be between 1 and 12 characters long, without spaces or \
    symbols.";
    std::string EXIT_MESSAGE = "Exiting the program...";
    
    // validates launch arguments
    std::string error_description = "";
    if(!is_valid_username(username_))
    {
        error_description = "Invalid username! It must be between 1 and 12 characters long, \
            without spaces or symbols.";
    }
    else if(!is_valid_address(server_address))
    {
        error_description = "Invalid IP address! The correct format should be XXX.XXX.XXX.XXX, \
            where X represents a number from 0 to 255, and there should be dots (.) separating the sections.";
    }
    else if(!is_valid_port(server_port))
    {
        error_description = "Invalid port! The correct port should be between 0 and 65535.";
    }    

    // stops initialization if some argument is not valid
    if(error_description != "")
    {   
        aprint(error_description);
        aprint(RUN_INFO);
        std::string ERROR_PARSING_CRITICAL = "Critical error parsing command-line options:";
        throw std::invalid_argument(ERROR_PARSING_CRITICAL);
    }

    try
    {
        // creates socket
        aprint("Creating socket...");
        connection_manager_.create_socket();

        // resolves host name
        aprint("Resolving host name...");
        std::string server_adjusted_address = connection_manager_.get_host_by_name(server_address);

        // tries to connect to server
        aprint("Attempting connection to server...");
        connection_manager_.connect_to_server(server_address, server_port);

        // after being connected tries to send login request
        aprint("Logging in as \'" + username + "\'...");
        session_id_ = connection_manager_.login(username_, machine_name_);
        aprint("Got following session id: " + std::to_string(session_id_));

        // sets running flag to true
        running_app_.store(true);

        // initializes user interface last
        UI_.start();
    }
    catch(const std::exception& e)
    {   
        // propagates error
        raise("Critical error intializing:\n\t\t" + std::string(e.what()));
    }
    
};

Client::~Client()
{
    // TODO: kill child threads here
    // TODO: close connection here
    close();
};

      
void Client::start_sync_(std::string new_path) 
{
    // initializes synchronization process on client, informs server
    // by sending "clist" command to server
    // it is impled that client is requesting file updates on both ends
    // sync dir path is optional
    
    if(running_sync_.load() == false)
    {
        // checks if the download directories were previously set
        // if not, uses the default path
        if(sync_dir_path_.size() == 0)
        {
            this->sync_dir_path_ = this->default_sync_dir_path_;
        }
        if(async_dir_path_.size() == 0)
        {
            this->async_dir_path_ = this->default_async_dir_path_;
        }

        // checks for system paths permissions on the given folders
        if(!is_valid_path(sync_dir_path_))
        {
            raise("Given sync dir path is not valid!");
        }
        if(!is_valid_path(async_dir_path_))
        {
            raise("Given async dir path is not valid!");
        }

        // checks for temporary download files and removes them
        int deleted_sync_temp_files = delete_temporary_download_files_(sync_dir_path_);
        int deleted_async_temp_files = delete_temporary_download_files_(async_dir_path_);
        if(deleted_sync_temp_files > 0)
        {
            std::string output = "Deleted " + std::to_string(deleted_sync_temp_files);
            output += " incomplete download files from the sync dir.";
            aprint(output);
        }
        else
        {
            std::string output = "No incomplete files were found on the sync dir.";
            aprint(output);
        }
        if(deleted_async_temp_files > 0)
        {
            std::string output = "Deleted " + std::to_string(deleted_sync_temp_files);
            output += " incomplete download files from the async dir.";
            aprint(output);
        }
        else
        {
            std::string output = "No incomplete files were found on the async dir.";
        }

        // initializes inotify watcher module
        inotify_.init(
            sync_dir_path_, 
            inotify_buffer_, 
            inotify_buffer_mtx_);
        inotify_.start_watching();
        
        aprint("Synchronization routine initialized!");
        return;
    }
    else
    {
        aprint("Synchronization routine is already running!");
        return;
    }

    // starts synchronization routine, setting the new sync_dir path if needed
    if(!is_valid_path(new_path))
    {
       if(!create_directory(new_path))
       {
            raise("Could not acess informed sync dir path!");
       }
    }

    // sync dir is valid and acessible
    
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
        std::string output = "Exception occured on main_loop():\n\t\t" + std::string(e.what());
        raise(output);
    }
    catch (...) 
    {
        raise("Unknown error occured on main_loop()!");
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
        raise("Exception occured on start():\n\t\t" + std::string(e.what()));
    }
    catch(...)
    {
        raise("Unknown error occured on start()!");
    }
    
}

void Client::stop()
{
    // stop user interface and command processing
    try
    {
        aprint("Exiting the program...");

        running_app_.store(false);
        UI_.stop();
        ui_cv_.notify_all();

    }
    catch(const std::exception& e)
    {
        raise("Error occured on stop(): " + std::string(e.what()));
    }
    catch(...)
    {
        raise("Unknown error occured on stop()!");
    }
}

void Client::close()
{
    // closes remaining threads
    UI_.input_thread_.join();
}

void client_app::aprint(std::string content, int scope, bool endl)
{
    std::string scope_name = "";
    switch (scope)
    {
        case 1:
        {
            scope_name = " USER INTERFACE COMMANDS";
            break;
        }
        case 2:
        {
            scope_name = " NETWORK COMMANDS";
            break;
        }
        case 3:
        {
            scope_name = " CLIENT SIDE NETWORK COMMANDS";
            break;
        }
        case 4:
        {
            scope_name = " SERVER SIDE NETWORK COMMANDS";
            break;
        }
        default:
            break;
    }

    std::string full_scope_name = "syncwizard client" + scope_name;
    print(content, full_scope_name, 1, 1, 2, endl);
}

void client_app::raise(std::string error, int scope)
{
    std::string scope_name = "";
    switch (scope)
    {
        case 1:
        {
            scope_name = " USER INTERFACE COMMANDS";
            break;
        }
        case 2:
        {
            scope_name = " NETWORK COMMANDS";
            break;
        }
        case 3:
        {
            scope_name = " CLIENT SIDE NETWORK COMMANDS";
            break;
        }
        case 4:
        {
            scope_name = " SERVER SIDE NETWORK COMMANDS";
            break;
        }
        default:
            break;
    }
    throw std::runtime_error("[CLIENT APP" + scope_name + "] " + error);
}
