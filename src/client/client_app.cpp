// c++
#include <iostream>
#include <exception>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <fstream>

// multithreading & synchronization
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>

#include <cstring>

// argument parsing
#include "../include/common/cxxopts.hpp"

// local
#include "client_app.hpp"
#include "../include/common/user_interface.hpp"
#include "../include/common/connection.hpp"
#include "../include/common/utils.hpp"
#include "../include/common/inotify_watcher.hpp"

using namespace client_app;

Client::Client(const std::string& u, const std::string& add, const int& p)
    :   username_(u), 
        server_address_(add), 
        server_port_(p), 
        UI_(&mutex_, &cv_, &command_buffer, &sanitized_commands_),
        connection_(),
        running_(true),
        stop_requested_(false)
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
    const std::string HELP_DESCRIPTION = "This option displays the description of the available \
    program arguments. If you need help or have questions about how to use SyncWizard, you \
    can use this option to get more information.";
    const std::string RUN_INFO = "Please run './SyncWizard -h' for more information.";
    const std::string USERNAME_DESCRIPTION = "Use this option to specify the username to which \
    the files will be associated. This is important for identifying the synchronized files \
    belonging to each user. It must be between 1 and 12 characters long, without spaces or \
    symbols.";
    const std::string CONNECTING_TO_SERVER = "Attempting connection to server...";
    const std::string EXIT_MESSAGE = "Exiting the program...";
    
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
            async_utils::async_print("\t[SYNCWIZARD CLIENT] Could not find a valid command by \"" + command_buffer + "\"!");
            break;
        case 2:
            if(sanitized_commands_[0] == "get" && sanitized_commands_[1] == "sync_dir")
            {
                if(!inotify_.is_running())
                {
                    sync_dir_ = "./sync_dir";
                    inotify_.init(sync_dir_, inotify_buffer_, inotify_buffer_mtx_);
                    inotify_.start_watching();
                    async_utils::async_print("\t[SYNCWIZARD CLIENT] Synchronization routine initialized!");
                    break;
                }
                else
                {
                    async_utils::async_print("\t[SYNCWIZARD CLIENT] Synchronization already initialized!");
                    break;
                }
            }
            else if(sanitized_commands_[0] == "download")
            {
                // user requesting an upload
            }
            else if(sanitized_commands_[0] == "upload")
            {
                // user requesting a download
            }
            else
            {
                async_utils::async_print("\t[SYNCWIZARD CLIENT] Could not find a valid command by \"" + command_buffer + "\"!");
                break;
            }
            break;
        case 3:
            if(sanitized_commands_[0] == "get" && sanitized_commands_[1] == "sync" && sanitized_commands_[2] == "dir")
            {
                if(!inotify_.is_running())
                {
                    sync_dir_ = "./sync_dir";
                    inotify_.init(sync_dir_, inotify_buffer_, inotify_buffer_mtx_);
                    inotify_.start_watching();
                    async_utils::async_print("\t[SYNCWIZARD CLIENT] Synchronization routine initialized!");
                    break;
                }
                else
                {
                    async_utils::async_print("\t[SYNCWIZARD CLIENT] Synchronization already initialized!");
                    break;
                }
            }
            else
            {
                async_utils::async_print("\t[SYNCWIZARD CLIENT] Could not find a valid command by \"" + command_buffer + "\"!");
                break;
            }
        default:
            async_utils::async_print("\t[SYNCWIZARD CLIENT] Could not find a valid command by \"" + command_buffer + "\"!");
            break;
    }

    int inotify_nargs = inotify_buffer_.size();

    switch (inotify_nargs)
    {
    case 3:
        std::string type = inotify_buffer_[1];
        std::string file_path = inotify_buffer_[2];
        if(type == "create" || type == "modify")
        {
            // upload file to server
        }
        else if(type == "delete")
        {
            // request file deletion on server
        }
        break;
    
    default:
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
                cv_.wait(lock, [this]{return command_buffer.size() > 0 || stop_requested_.load();});
            }

            // checks again if the client should be running
            if(stop_requested_.load())
            {
                cv_.notify_one();
                break;
            }

            process_input();

            // resets shared buffers
            command_buffer.clear();
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
        async_utils::async_print("\t[SYNCWIZARD CLIENT] Exiting the program...");

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

void Client::send(char* buffer, std::size_t buffer_size, int timeout)
{
    if(timeout != -1)
    {
        connection_.send_data(buffer, buffer_size, connection_.get_sock_fd(), timeout);
    }
    else
    {
        connection_.send_data(buffer, buffer_size, connection_.get_sock_fd());
    }
}

void Client::recieve(char* buffer, std::size_t buffer_size, int timeout)
{
    if(timeout != -1)
    {
        connection_.recieve_data(buffer, buffer_size, connection_.get_sock_fd(), timeout);
    }
    else
    {
        connection_.recieve_data(buffer, buffer_size, connection_.get_sock_fd());
    }
}


void Client::start_receiver()
{
    running_receiver_.store(true);
    receiver_th_ = std::thread(
        [this]()
        {
            receiver_loop();
        });
}

void Client::stop_receiver()
{
    if(receiver_th_.joinable())
    {
        receiver_th_.join();
    }
}

void Client::receiver_loop()
{
    running_receiver_.store(true);
    while(running_receiver_.load())
    {
        std::unique_lock<std::mutex> lock(recieve_mtx_);
        if (!running_receiver_.load())
        {
            async_utils::async_print("[\tCLIENT APP] Sender stop requested!");
            running_receiver_.store(false);
            return;
        }

        try
        {
            char buffer[1024];
            this->recieve(buffer, sizeof(buffer));
            std::vector<std::string> received_buffer = split_buffer(buffer);
            int nargs = received_buffer.size();

            switch(nargs)
            {
                case 0:
                {
                    async_utils::async_print("\t[CLIENT APP] Received malformed string from server.");
                    break;
                }
                case 1:
                {
                    std::string command_name = received_buffer.front();
                    if(command_name == "ping")
                    {
                        char buffer[1024] = "pong";
                        std::unique_lock<std::mutex> lock(send_mtx_);
                        sender_buffer_.push_back(std::make_pair(buffer, sizeof(buffer)));
                        break;
                    }
                    else if(command_name == "pong")
                    {
                        auto ping_end = std::chrono::high_resolution_clock::now();
                        auto ping_val = std::chrono::duration_cast<std::chrono::microseconds>(ping_end - ping_start_);
                        double ping_ms = ping_val.count() / 1000.0; // Em milissegundos
                        std::string output = "\t[CLIENT APP] Pinged server with a response time of " + std::to_string(ping_ms) + "ms.";
                        async_utils::async_print(output);
                        break;
                    }
                    else
                    {
                        async_utils::async_print("\t[CLIENT APP] Received malformed \"" + command_name + "\" command from server!");
                        break;
                    }
                }
                case 2:
                { 
                    std::string command_name = received_buffer[0];
                    std::string args = received_buffer[1];

                    if(command_name == "delete")
                    {
                        args = sync_dir_ + args;

                        if(!is_valid_path(args))
                        {
                            {
                                std::unique_lock<std::mutex> lock(send_mtx_);
                                char buffer[1024] = "delete|fail";
                                sender_buffer_.push_back(std::make_pair(buffer, sizeof(buffer)));
                            }
                            async_utils::async_print("\t[CLIENT APP] Invalid file path. Delete command ignored.");
                            break;
                        }
                        else if(received_buffer[1] == "fail")
                        {
                            async_utils::async_print("\t[CLIENT APP] Server could not process malformed delete command!");
                            break;
                        }
                        else
                        {
                            // valid path, deletes file
                            try
                            {
                                delete_file(args);
                                std::unique_lock<std::mutex> lock(send_mtx_);
                                char buffer[1024] = "delete|ok";
                                sender_buffer_.push_back(std::make_pair(buffer, sizeof(buffer)));
                                break;
                            }
                            catch(const std::exception& e)
                            {
                                async_utils::async_print("\t[CLIENT APP] Exception raised while deleting file: " + std::string(e.what()));
                                std::unique_lock<std::mutex> lock(send_mtx_);
                                char buffer[1024] = "delete|fail";
                                sender_buffer_.push_back(std::make_pair(buffer, sizeof(buffer)));
                                
                                // TODO: request download for this file!
                                break;
                            }
                            
                        }
                        break;
                    }
                    else if(command_name == "exit")
                    {
                        // received logout confirmation from server
                        async_utils::async_print("\t[CLIENT APP] Logout requested by server!");
                        std::unique_lock<std::mutex> lock(send_mtx_);
                        char logout_buffer[1024] = "logout|ok";
                        this->send(logout_buffer, sizeof(logout_buffer));
                        running_receiver_.store(false);
                        running_sender_.store(false);
                        stop();
                        return;
                    }
                    else if(command_name == "download")
                    {
                        // server is requesting some file
                        std::string file_path = args;
                        file_path = sync_dir_ + file_path;

                        if(!is_valid_path(file_path))
                        {
                            
                            std::unique_lock<std::mutex> lock(send_mtx_);
                            char buffer[1024] = "upload|fail";
                            sender_buffer_.push_back(std::make_pair(buffer, sizeof(buffer)));
                            async_utils::async_print("\t[CLIENT APP] Malformed upload requested by server!");
                            async_utils::async_print("\t[CLIENT APP] Could not acess given file!");
                            break;
                        }
                        else
                        {
                            std::string checksum = calculate_md5_checksum(file_path);
                            std::ifstream file(file_path, std::ios::binary);

                            if (!file.is_open()) 
                            {
                                async_utils::async_print("\t[CLIENT APP] Could not acess given file!");
                                std::unique_lock<std::mutex> lock(send_mtx_);
                                char buffer[1024] = "upload|fail";
                                sender_buffer_.push_back(std::make_pair(buffer, sizeof(buffer)));
                                break;
                            }

                            {
                                std::unique_lock<std::mutex> lock(send_mtx_);
                                while(!file.eof())
                                {
                                    std::string buffer_s = "upload|" + received_buffer[1] + "|" + checksum + "|";
                                    char file_buffer[sizeof(buffer) - sizeof(buffer_s)];
                                    file.read(file_buffer, sizeof(file_buffer));
                                    
                                    // mounts packet
                                    buffer_s += std::string(file_buffer);
                                    
                                    char send_buf[1024];
                                    strncpy(send_buf, buffer_s.c_str(), sizeof(send_buf));
                                    send_buf[sizeof(send_buf) - 1] = '\0';
                                    sender_buffer_.push_back(std::make_pair(send_buf, sizeof(send_buf)));
                                }
                                file.close();

                                // sends EOF packet
                                std::string buffer_s = "upload|" + received_buffer[1] + "|" + checksum + "|";
                                char file_buffer[sizeof(buffer) - sizeof(buffer_s)];
                                for(size_t i = sizeof(buffer_s); i < sizeof(buffer); ++i) 
                                {
                                    file_buffer[i] = 1;
                                }

                                strncpy(buffer, buffer_s.c_str(), sizeof(buffer));
                                buffer[sizeof(buffer) - 1] = '\0';

                                sender_buffer_.push_back(std::make_pair(buffer, sizeof(buffer)));
                            }
                            break;
                        }
                    }
                    else
                    {
                        async_utils::async_print("\t[CLIENT APP] Received malformed \"" + command_name + "\" command from server!");
                        break;
                    }
                }
                case 3:
                {
                    std::string command_name = received_buffer[0];
                    std::string args = received_buffer[1];
                    int payload_index = sizeof(buffer) - sizeof(command_name) - sizeof(args);
                    char payload[payload_index];
                    
                    // checks for end of command payload
                    bool is_eof = true;
                    for (size_t i = payload_index; i < sizeof(buffer); ++i) 
                    {
                        payload[i] = buffer[i];
                        
                        if(buffer[i] != 1)
                        {
                            is_eof = false;
                        }
                    }

                    if(command_name == "list")
                    {
                        if(args == "server")
                        {
                            if(file_listing_string_.empty() && !is_eof)
                            {
                                file_listing_string_ += "\t[CLIENT APP] Currently these files are being hosted for your user:";
                            }
                            else if(!is_eof)
                            {
                                file_listing_string_ += std::string(payload);
                            }
                            else if(!file_listing_string_.empty() && is_eof)
                            {
                                async_utils::async_print(file_listing_string_);
                                file_listing_string_.clear();
                            }
                            else
                            {
                                async_utils::async_print("[\tCLIENT APP] Found no files for your user!");
                            }
                            break;
                        }
                        else
                        {
                            async_utils::async_print("\t[CLIENT APP] List command failed!");
                            file_listing_string_.clear();
                            break;
                        }
                    }
                    else
                    {
                        async_utils::async_print("\t[CLIENT APP] Received malformed \"" + command_name + "\" command from server!");
                        break;
                    }
                }
                case 4:
                {
                    std::string command_name = received_buffer[0];
                    std::string file_path = received_buffer[1];
                    std::string checksum = received_buffer[2];
                    int payload_index = sizeof(buffer) - sizeof(command_name) - sizeof(file_path) - sizeof(checksum);

                    // fills payload
                    char payload[payload_index];
                    bool is_eof = true;
                    for (size_t i = payload_index; i < sizeof(buffer); ++i) 
                    {
                        payload[i] = buffer[i];
                        
                        if(buffer[i] != 1)
                        {
                            is_eof = false;
                        }
                    }

                    if(command_name == "upload")
                    {
                        // server is sending some file
            
                        std::string file_path = sync_dir_ + file_path;
                        std::string temp_file_path = file_path + ".tmp";

                        if(!is_eof)
                        {
                            std::ofstream temp_file(temp_file_path, std::ios::app | std::ios::binary);
                            if(!temp_file) 
                            {
                                std::unique_lock<std::mutex> lock(send_mtx_);
                                char buffer[1024] = "upload|fail";
                                sender_buffer_.push_back(std::make_pair(buffer, sizeof(buffer)));
                                async_utils::async_print("\t[CLIENT APP] Malformed download requested by server!");
                                async_utils::async_print("\t[CLIENT APP] Could not acess given file!");
                                break;
                            }
                            
                            // updates temporary file with payload
                            temp_file.write(payload, sizeof(payload));
                            temp_file.close();

                            break;
                        }
                        else
                        {
                            // end of file reached, checks checksum value, then replaces original file
                            std::string current_checksum = calculate_md5_checksum(file_path);
                            
                            if(current_checksum != checksum)
                            {
                                async_utils::async_print("\t[CLIENT APP] End of file reached, but temp checksum does not match ->" + file_path);
                            }

                            // ensures nothing is being uploaded at the moment before running
                            {
                                std::unique_lock<std::mutex> lock(send_mtx_);
                                send_cv_.wait(lock, [this]() { return !sender_buffer_.empty() || !running_sender_.load(); });

                                async_utils::async_print("\t[CLIENT APP] Temporary file complete: " + file_path + " overwritting original...");
                                rename_replacing(temp_file_path, file_path);
                            }

                            break;
                        }
                    }
                    else
                    {
                        async_utils::async_print("\t[CLIENT APP] Received malformed \"" + command_name + "\" command from server!");
                        break;
                    }
                }
                default:
                {
                    std::string command_name = received_buffer.front();
                    async_utils::async_print("\t[CLIENT APP] Received malformed \"" + command_name + "\" command from server!");
                    break;
                }
            }
        }
        catch(const std::exception& e)
        {
            throw std::runtime_error("[CLIENT APP] Error recieving data on user " + username_ + ": " + std::string(e.what()));
        }
    }
}

void Client::start_sender()
{
    running_sender_.store(true);
    sender_th_ = std::thread(
        [this]()
        {
            sender_loop();
        });
}

void Client::stop_sender()
{
    running_sender_.store(false);
    if(sender_th_.joinable())
    {
        sender_th_.join();
    }
}

void Client::sender_loop()
{
    running_sender_.store(true);
    while(running_sender_.load())
    {
        try
        {
            std::unique_lock<std::mutex> lock(send_mtx_);
            send_cv_.wait(lock, [this]() { return !sender_buffer_.empty() || !running_sender_.load(); });

            if(!running_sender_.load())
            {
                running_sender_.store(false);
                return;
            }

            if(!sender_buffer_.empty())
            {
                auto data = sender_buffer_.front();
                sender_buffer_.pop_front();
                
                // sends using previoulsy set callback method
                this->send(data.first, data.second);
            }

        }
        catch(const std::exception& e)
        {
            throw std::runtime_error("[CLIENT APP] Exception occured while running send: " + std::string(e.what()));
        }

        send_cv_.notify_one();
    }
}

