// c++
#include <iostream>
#include <string>
#include <cstring>
#include <chrono>
#include <vector>
#include <exception>
#include <stdexcept>
#include <filesystem>
#include <vector>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fstream>
#include <dirent.h>

// multithread & synchronization
#include <thread>
#include <mutex>
#include <condition_variable>

// locals
#include "client_connection.hpp"
#include "../include/common/json.hpp"
#include "../include/common/utils.hpp"

using namespace client_connection;
namespace fs = std::filesystem;
using json = nlohmann::json;

ClientSession::ClientSession(
    int sock_fd, 
    std::string username,
    std::string machine_name,
    std::string directory_path,
    std::function<void(char* buffer, std::size_t buffer_size, int sock_fd, int timeout)> send_callback,
    std::function<void(char* buffer, std::size_t buffer_size, int sock_fd, int timeout)> recieve_callback,
    std::function<void(char* buffer, std::size_t buffer_size)> broadcast_user_callback)
    :   socket_fd_(sock_fd),
        username_(username),
        machine_(machine_name),
        directory_path_(directory_path),
        send_callback_(send_callback),
        recieve_callback_(recieve_callback),
        broadcast_user_callback_(broadcast_user_callback),
        running_receiver_(false),
        running_sender_(false),
        initializing_(true)
{
    async_utils::async_print("\t[SESSION MANAGER] Starting up new session for user " + username_ + "(" + std::to_string(socket_fd_) + ")");
    start_sender();
    start_receiver();
    async_utils::async_print("\t[SESSION MANAGER] Started!");
}

ClientSession::~ClientSession()
{
    stop_receiver();
    stop_sender();
}

int ClientSession::get_socket_fd()
{
    return socket_fd_;
}

std::string ClientSession::get_username()
{
    return username_;
}

std::string ClientSession::get_address()
{
    return address_;
}

std::string ClientSession::get_machine_name()
{
    return machine_;
}

void ClientSession::start_receiver()
{
    running_receiver_.store(true);
    receiver_th_ = std::thread(
        [this]()
        {
            session_receiver_loop();
        });
}

void ClientSession::stop_receiver()
{
    if(receiver_th_.joinable())
    {
        receiver_th_.join();
    }
}

void ClientSession::session_receiver_loop()
{
    running_receiver_.store(true);
    while(running_receiver_.load())
    {
        std::unique_lock<std::mutex> lock(recieve_mtx_);
        if (!running_receiver_.load())
        {
            async_utils::async_print("[SESSION MANAGER] Sender stop requested!");
            running_receiver_.store(false);
            return;
        }

        try
        {
            char buffer[1024];
            this->recieve(buffer, sizeof(buffer));
            std::list<std::string> command_buffer = split_buffer(buffer);
            switch(command_buffer.size())
            {
                case 0:
                {
                    std::string strout = "\t[SESSION MANAGER] Recieved empty string from user \"";
                    strout += username_ + "\" at session " + std::to_string(socket_fd_) + ".";
                    async_utils::async_print(strout);
                    break;
                }
                case 1:
                {
                    std::string command_name = command_buffer.back();
                    if(command_name == "exit")
                    {
                        async_utils::async_print("\t[SESSION MANAGER] User \"" + username_ + "\" requested a logout.");

                        // sends logout confirmation to user
                        std::unique_lock<std::mutex> lock(send_mtx_);
                        this->disconnect("exit|ok");
                        running_receiver_.store(false);
                        running_sender_.store(false);
                        running_handler_.store(false);
                        std::string output("\t[SESSION MANAGER] User \"" + username_ + "\" session: ");
                        output += std::to_string(socket_fd_) + " logged out.";
                        break;
                    }
                    else if(command_name == "ping")
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
                        std::string output = "\t[SESSION MANAGER] Pinged user \"" + username_ + "\" session ";
                        output += std::to_string(socket_fd_) + " -> " + std::to_string(ping_ms) + "ms.";
                        async_utils::async_print(output);
                        break;
                    }
                    else
                    {
                        std::string output = "\t[SESSION MANAGER] Recieved invalid command from user \"";
                        output += username_ + "\" at session " + std::to_string(socket_fd_) + ".";
                        async_utils::async_print(output);
                        break;
                    }
                }
                case 2:
                { 
                    std::string command_name = command_buffer[0];
                    std::string file_path = command_buffer[1];

                    if(command_name == "delete")
                    {
                        if(file_path.begin() == "\"" || file_path.begin() == "\'")
                        {
                            file_path.erase(0);
                        }
                        if(file_path.back() == "\"" || file_path.back() == "\'")
                        {
                            file_path.pop_back();
                        }
                        file_path = directory_path_ + file_path;

                        if(!is_valid_path(file_path))
                        {
                            {
                                std::unique_lock<std::mutex> lock(send_mtx_);
                                char buffer[1024] = "failed";
                                sender_buffer_.push_back(std::make_pair(buffer, sizeof(buffer)));
                            }
                            async_utils::async_print("[SESSION MANAGER] Invalid file path. Delete command ignored.");
                            break;
                        }
                        else
                        {
                            // valid path, deletes file
                            try
                            {
                                delete_file(file_path);
                                char buffer[1024] = "delete|ok";
                                std::unique_lock<std::mutex> lock(send_mtx_);
                                sender_buffer_.push_back(std::make_pair(buffer, sizeof(buffer)));

                                break;
                            }
                            catch(const std::exception& e)
                            {
                                std::string err = "[SESSION MANAGER] Exception raised while deleting file on user "
                                    + username_ + " session " + std::to_string(socket_fd_) + ". ->" + std::string(e.what());
                                throw std::runtime_error(err);
                                break;
                            }
                            
                        }
                        break;
                    }
                    else if(command_buffer.front() == "list")
                    {
                        if(command_buffer.back() == "server")
                        {
                            std::string output = "\t[SESSION MANAGER] Currently these files are being hosted for your user:";
                            
                            DIR* dir = opendir(directory_path_.c_str());
                            if(dir == nullptr) 
                            {
                                throw std::runtime_error("[SESSION MANAGER] Could not acess user folder for user " + username_ +
                                    " at session " + std::to_string(socket_fd_));
                                return;
                            }

                            std::function<void(const std::string&)> list_files_recursively = [&](const std::string& current_path) 
                            {
                                DIR* dir = opendir(current_path.c_str());
                                if(dir == nullptr) 
                                {
                                    throw std::runtime_error("[SESSION MANAGER] Could not access user folder for user " + username_ +
                                                            " at session " + std::to_string(socket_fd_));
                                    return;
                                }

                                dirent* entry;
                                while((entry = readdir(dir)) != nullptr) 
                                {
                                    if(entry->d_type == DT_REG) 
                                    { 
                                        std::string file_path = current_path + "/" + entry->d_name;
                                        file_path += "/";
                                        file_path += entry->d_name;

                                        struct stat file_info;
                                        if(lstat(file_path.c_str(), &file_info) == 0) 
                                        {
                                            char modification_time_buffer[100];
                                            char access_time_buffer[100];
                                            char change_creation_time_buffer[100];

                                            output += "\n\t\tFile name: " + std::string(entry->d_name);
                                            output += "\n\t\tFile path: " + file_path;

                                            std::strftime(modification_time_buffer, sizeof(modification_time_buffer), "%c", std::localtime(&file_info.st_mtime));
                                            output += "\n\t\tModification time: " + std::string(modification_time_buffer);

                                            std::strftime(access_time_buffer, sizeof(access_time_buffer), "%c", std::localtime(&file_info.st_atime));
                                            output += "\n\t\tAccess time: " + std::string(access_time_buffer);

                                            std::strftime(change_creation_time_buffer, sizeof(change_creation_time_buffer), "%c", std::localtime(&file_info.st_ctime));
                                            output += "\n\t\tChange/creation time: " + std::string(change_creation_time_buffer);

                                        }
                                        else if(entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) 
                                        {
                                            list_files_recursively(current_path + "/" + entry->d_name);
                                        }
                                    }
                                }
                                closedir(dir);
                            };

                            list_files_recursively(directory_path_);
                            async_utils::async_print(output);
                            break;
                        }
                        else
                        {
                            std::string output = "\t[SESSION MANAGER] Recieved invalid command from user \"";
                            output += username_ + "\" at session " + std::to_string(socket_fd_) + ".";
                            {
                                std::unique_lock<std::mutex> lock(send_mtx_);
                                char buffer[1024] = "failed";
                                sender_buffer_.push_back(std::make_pair(buffer, sizeof(buffer)));
                            }
                            async_utils::async_print(output);
                            break;
                        }
                    }
                    else
                    {
                        std::string output = "\t[SESSION MANAGER] Recieved invalid command from user \"";
                        output += username_ + "\" at session " + std::to_string(socket_fd_) + ".";
                        async_utils::async_print(output);
                        break;
                    }
                }
                case 4:
                {
                    std::string command_name = command_buffer[0];
                    std::string file_path = command_buffer[1];
                    std::string checksum = command_buffer[2];
                    int payload_index = sizeof(buffer) - sizeof(command_name) - sizeof(file_path) - sizeof(checksum);
                    char payload[payload_index];

                    // fills payload
                    bool is_eof = true;
                    for (size_t i = payload_index; i < sizeof(buffer); ++i) 
                    {
                        payload[i] = buffer[i];
                        
                        if(buffer[i] != 1)
                        {
                            is_eof = false;
                        }
                    }

                    if(command_name == "download")
                    {
                        std::string file_name = get_file_name(file_path);

                        // ensures string is just file path and not "filepath"
                        if(file_path.back() == "\"" || file_path.back() == "\'")
                        {
                            file_path.pop_back();
                        }
                        if(file_path.front() == "\"" || file_path.front() == "\'")
                        {
                            file_path.erase(0);
                        }
                        
                        file_path = directory_path_ + file_path;
                        if(!is_valid_path(file_path))
                        {
                            {
                                std::unique_lock<std::mutex> lock(send_mtx_);
                                char buffer[1024] = "fail";
                                sender_buffer_.push_back(std::make_pair(buffer, sizeof(buffer)));
                            }
                            break;
                        }
                        else
                        {
                            std::string checksum = calculate_md5_checksum(file_path);
                            std::ifstream file(file_path, std::ios::binary);

                            if (!file.is_open()) 
                            {
                                throw std::runtime_error("[SESSION MANAGER] Could not open given file!");
                            }

                            // tells the user to expect to recieve the following file
                            {
                                std::unique_lock<std::mutex> lock(send_mtx_);
                                std::string buffer_s = "downloading|" + command_buffer[1] + "|" + checksum + "|";
                                while(!file.eof())
                                {
                                    char file_buffer[sizeof(buffer) - sizeof(buffer_s)];
                                    file.read(file_buffer, sizeof(file_buffer));
                                    
                                    // mounts packet
                                    buffer_s += std::string(file_buffer);
                                    buffer = buffer_s.c_str();
                                    sender_buffer_.push_back(std::make_pair(buffer, sizeof(buffer)));
                                }
                                file.close();

                                // sends EOF packet
                                std::string buffer_s = "downloading|" + command_buffer[1] + "|" + checksum + "|";
                                for(size_t i = sizeof(buffer_s); i < sizeof(buffer); ++i) 
                                {
                                    file_buffer[i] = 1;
                                }
                                sender_buffer_.push_back(std::make_pair(buffer, sizeof(buffer)));
                            }
                            break;
                        }
                    }
                    if(command_name == "upload")
                    {
                        std::string file_path = directory_path_ + file_path;
                        std::string temp_file_path = file_path + ".tmp";

                        if(!is_eof)
                        {
                            std::ofstream temp_file(temp_file_path, std::ios::app | std::ios::binary);
                            if(!temp_file) 
                            {
                                throw std::runtime_error("[SESSION MANAGER] Could not open given file!");
                                break;
                            }
                            
                            temp_file.write(payload.c_str(), payload.size());
                            temp_file.close();
                        }
                        else
                        {
                            // end of file reached, checks checksum value, then replaces original file
                            std::string current_checksum = calculate_md5_checksum(file_path);
                            
                            if(current_checksum != checksum)
                            {
                                async_utils::async_print("[SESSION MANAGER] End of file reached, but temp checksum does not match ->" + file_path);
                            }
                            rename_replacing(temp_file_path, file_path);
                        }
                    }
                    break;
                }
                default:
                    std::string output = "\t[SESSION MANAGER] Recieved invalid command from user \"";
                    output += username_ + "\" at session " + std::to_string(socket_fd_) + ".";
                    async_utils::async_print(output);
                    {
                        std::unique_lock<std::mutex> lock(send_mtx_);
                        char buffer[1024] = "failed";
                        sender_buffer_.push_back(std::make_pair(buffer, sizeof(buffer)));
                    }
                    break;
            }
        }
        catch(const std::exception& e)
        {
            throw std::runtime_error("[SESSION MANAGER] Error recieving data on user " + username_ + ": " + std::string(e.what()));
        }
    }
}

void ClientSession::start_sender()
{
    running_sender_.store(true);
    sender_th_ = std::thread(
        [this]()
        {
            session_sender_loop();
        });
}

void ClientSession::stop_sender()
{
    running_sender_.store(false);
    if(sender_th_.joinable())
    {
        sender_th_.join();
    }
}

void ClientSession::session_sender_loop()
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
            throw std::runtime_error("[SESSION MANAGER] Exception occured while running send: " + std::string(e.what()));
        }

        send_cv_.notify_one();
    }
}

void ClientSession::send(char* buffer, std::size_t buffer_size, int timeout)
{
    // sends a buffer along with its own socket descriptor
    std::lock_guard<std::mutex> lock(send_mtx_);
    send_callback_(buffer, buffer_size, socket_fd_, timeout);
}

void ClientSession::recieve(char* buffer, std::size_t buffer_size, int timeout)
{
    std::lock_guard<std::mutex> lock(recieve_mtx_);
    recieve_callback_(buffer, buffer_size, socket_fd_, timeout);
}

void ClientSession::send_ping()
{
    ping_start_ = std::chrono::high_resolution_clock::now();
    char ping_buffer[] = "ping";
    send(ping_buffer, sizeof(ping_buffer));
}

void ClientSession::disconnect(std::string reason)
{
    char disconnect_buffer[1024];
    std::string disconnect_string = "logout|" + reason;
    strcpy(disconnect_buffer, disconnect_string.c_str());

    // informs the user about the disconnection
    send(disconnect_buffer, sizeof(disconnect_buffer));

    stop_handler();
    stop_receiver();
    stop_sender();

    close(socket_fd_);
}