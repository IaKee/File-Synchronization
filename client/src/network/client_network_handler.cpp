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

// local
#include "client_app.hpp"
#include "../include/common/connection_manager.hpp"
#include "../include/common/async_cout.hpp"
#include "../include/common/utils.hpp"
#include "../include/common/utils_packet.hpp"

using namespace client_app;
using namespace async_cout;

void Client::start_receiver()
{
    aprint("Starting up receiver module.", 2);
    running_receiver_.store(true);
    receiver_th_ = std::thread(
        [this]()
        {
            receiver_loop();
        });
}

void Client::stop_receiver()
{
    aprint("Stopping receiver module...", 2);
    running_receiver_.store(false);
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
        std::unique_lock<std::mutex> lock(receive_mtx_);
        if (running_receiver_.load() == false)
        {
            aprint("Stopping receiver module...", 2);
            return;
        }   

        try
        {
            // receives a new packet
            packet buffer;
            connection_manager_.receive_packet(&buffer);

            // sanitizes packet command argument
            std::vector<std::string> received_buffer = split_buffer(buffer.command);
            int nargs = received_buffer.size();

            switch(nargs)
            {
                case 0:
                {
                    aprint("Received malformed string from server.", 2);
                    break;
                }
                case 1:
                {
                    std::string command_name = received_buffer.front();
                    if(command_name == "ping")
                    {
                        // server is pinging local user
                        this->server_ping_command_();
                        break;
                    }
                    else if(command_name == "pong")
                    {
                        // server is responding to a previous user ping request
                        this->pong_command_();
                        break;
                    }
                    else
                    {
                        this->server_malformed_command_(command_name);
                        break;
                    }
                }
                case 2:
                { 
                    std::string command_name = received_buffer[0];
                    std::string args = received_buffer[1];

                    if(command_name == "delete")
                    {
                        // received delete request from server
                        this->server_delete_file_command_(args, buffer);
                        break;
                    }
                    else if(command_name == "exit")
                    {
                        // recieved exit request from server
                        std::string reason = charraystr(buffer.payload, buffer.payload_size);
                        server_exit_command_(reason);
                        break;
                    }
                    else if(command_name == "sdownload")
                    {
                        // recieved download request from server
                        // server does not have some file
                        this->server_download_command_(args);
                        break;
                    }
                    else if(command_name == "list")
                    {
                        // recieved list request from server
                        this->server_list_command_(args, buffer);
                        break;
                    }
                    else
                    {
                        this->malformed_command_(command_name);
                        break;
                    }
                }
                case 3:
                {
                    std::string command_name = received_buffer[0];
                    std::string args = received_buffer[1];
                    std::string checksum = received_buffer[2];
            
                    if(command_name == "supload")
                    {
                        // server is sending some file
                        this->server_upload_command_(args, checksum, buffer);
                        break;
                    }   
                    else if(command_name == "aupload")
                    {
                        // server is sending some file to async download folder
                        this->server_async_upload_command_(args, checksum, buffer);
                        break;
                    }
                    else if(command_name == "upload")
                    {
                        // server is telling user that a previous upload request has failed!
                        this->server_upload_command_(args, checksum, buffer);
                        break;
                    }
                    else if(command_name == "delete")
                    {
                        // server is telling that a delete request has failed
                        this->server_delete_file_command_(args, buffer, checksum);
                        break;
                    }
                    else
                    {
                        this->malformed_command_(command_name);
                        break;
                    }
                }
                default:
                {
                    std::string command_name = received_buffer.front();
                    this->malformed_command_(command_name);
                    break;
                }
            }
        }
        catch(const std::exception& e)
        {
            std::string output = "Critical error recieving data from server: ";
            output += std::string(e.what());
            raise(output, 2);
        }
    }
}

void Client::start_sender()
{
    aprint("Starting up sender module.", 2);
    running_sender_.store(true);
    sender_th_ = std::thread(
        [this]()
        {
            sender_loop();
        });
}

void Client::stop_sender()
{
    aprint("Stopping sender module...", 2);
    running_sender_.store(false);
    send_cv_.notify_one();
    if(sender_th_.joinable())
    {
        sender_th_.join();
    }
}

void Client::sender_loop()
{
    running_sender_.store(true);
    while(running_sender_.load() == true)
    {
        try
        {
            std::unique_lock<std::mutex> lock(send_mtx_);
            send_cv_.wait(lock, [this]() { return !sender_buffer_.empty() || !running_sender_.load(); });

            if(running_sender_.load() == false)
            {
                aprint("Stopping sender module...", 2);
                return;
            }

            // process inotify events
            if(!inotify_buffer_.empty())
            {
                process_inotify_commands_();
            }

            if(sender_buffer_.empty() == false)
            {
                // sends using previoulsy set callback method - buffer is treated as FIFO
                connection_manager_.send_packet(sender_buffer_.front());

                sender_buffer_.erase(sender_buffer_.begin());
            }
        }
        catch(const std::exception& e)
        {
            std::string output = "Critical error sending data to server: ";
            output += std::string(e.what());
            raise(output, 2);
        }

        send_cv_.notify_one();
    }
}

