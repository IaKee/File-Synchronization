// standard c++
#include <iostream>
#include <filesystem>
#include <thread>
#include <mutex>

// c
#include <unistd.h>

// locals
#include "../client_connection.hpp"
#include "../../../../common/include/utils.hpp"
#include "../../../../common/include/asyncio/async_cout.hpp"

using namespace async_cout;
using namespace client_connection;
namespace fs = std::filesystem;

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
    running_receiver_.store(false);
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
        
        if (running_receiver_.load() == false)
        {
            aprint("Stopping sender...", 3);
            return;
        }

        try
        {
            // receives a new packet
            packet buffer;
            receive_packet_(&buffer, socket_fd_, -1);

            // sanitizes packet command argument
            std::vector<std::string> received_buffer = split_buffer(buffer.command);
            int nargs = received_buffer.size();

            switch(nargs)
            {
                case 0:
                {
                    // empty string, malformed command
                    this->malformed_command_(received_buffer.front());
                    break;
                }
                case 1:
                {
                    std::string command_name = received_buffer[0];

                    if(command_name == "exit")
                    {
                        this->client_requested_logout_();
                        break;
                    }
                    else if(command_name == "ping")
                    {
                        // received ping request from user
                        this->client_requested_ping_();
                        break;
                    }
                    else if(command_name == "pong")
                    {
                        // received ping response from user
                        this->client_responded_ping_();
                        break;
                    }
                    else if(command_name == "slist")
                    {
                        // user request a list of every file
                        // NOTE: user seems to never need this...
                        this->client_requested_slist_();
                        break;
                    }
                    else if(command_name == "clist")
                    {
                        // client sent a list of file currently on their local machine
                        // request updates to make server and session up to date
                        this->client_sent_clist_(buffer);
                        break;
                    }
                    else if (command_name == "get_sync_dir")
                    {
                        this->client_requested_get_sync_dir_();
                        break;
                    }
                    else
                    {
                        // malformed command
                        this->malformed_command_(command_name);
                        break;
                    }
                }
                case 2:
                { 
                    std::string command_name = received_buffer[0];
                    std::string args = received_buffer[1];

                    std::string local_file_path = directory_path_ + args;

                    if(command_name == "delete")
                    {
                        // user requested server to delete file
                        // propagates to other sessions
                        this->client_requested_delete_(args, buffer);
                        break;
                    }
                    else if(command_name == "flist")
                    {
                        // client requested formatted list of every file
                        this->client_requested_flist_();
                        break;
                    }
                    else if(command_name == "clist")
                    {
                        // client listing command probably failed
                        this->client_sent_clist_(buffer, args);
                        break;
                    }
                    else if(command_name == "download" || command_name == "adownload")
                    {
                        // user is requesting a file download to
                        // keep in a non synchronized folder
                        // send as "aupload"
                        this->client_requested_download_(args, 'a');
                        break;
                    }
                    else
                    {
                        // client sent malformed command
                        this->malformed_command_(command_name);
                        break;
                    }
                }
                case 3:
                {
                    std::string command_name = received_buffer[0];
                    std::string args = received_buffer[1];
                    std::string checksum = received_buffer[2];
                    
                    std::string local_file_path = directory_path_ + args;

                    if(command_name == "sdownload")
                    {
                        // user is sending some file
                        this->client_sent_sdownload_(args, buffer, checksum);
                        break;
                    }
                    else if(command_name == "upload")
                    {
                        // user sent back server file upload request
                        this->client_sent_supload_(args, buffer, checksum);
                        break;
                    }
                    else
                    {
                        // malformed command
                        this->malformed_command_(command_name);
                        break;
                    }
                }
                default:
                {
                    // malformed command
                    std::string command_name = received_buffer[0];
                    this->malformed_command_(command_name);
                    break;
                }
            }
        }
        catch(const std::exception& e)
        {
            std::string output = get_identifier() + " Error receiving data: ";
            output += std::string(e.what());
            raise(output, 2);
        }
    }
}

void ClientSession::add_packet_from_broadcast(packet& p)
{
    // receives a new packet
    packet buffer = p;
    receive_packet_(&buffer);

    // sanitizes packet command argument
    std::vector<std::string> received_buffer = split_buffer(buffer.command);
    int nargs = received_buffer.size();

    switch(nargs)
    {
        case 0:
        {
            // empty string, malformed command
            this->malformed_command_(received_buffer.front());
            break;
        }
        case 1:
        {
            std::string command_name = received_buffer[0];

            if(command_name == "exit")
            {
                this->client_requested_logout_();
                break;
            }
            else if(command_name == "ping")
            {
                // received ping request from user
                this->client_requested_ping_();
                break;
            }
            else if(command_name == "pong")
            {
                // received ping response from user
                this->client_responded_ping_();
                break;
            }
            else if(command_name == "slist")
            {
                // user request a list of every file
                // NOTE: user seems to never need this...
                this->client_requested_slist_();
                break;
            }
            else if(command_name == "flist")
            {
                // client requested formatted list of every file
                this->client_requested_flist_();
                break;
            }
            else if(command_name == "clist")
            {
                // client sent a list of file currently on their local machine
                // request updates to make server and session up to date
                this->client_sent_clist_(buffer);
                break;
            }
            else if (command_name == "get_sync_dir")
            {
                this->client_requested_get_sync_dir_();
                break;
            }
            else
            {
                // malformed command
                this->malformed_command_(command_name);
                break;
            }
        }
        case 2:
        { 
            std::string command_name = received_buffer[0];
            std::string args = received_buffer[1];

            std::string local_file_path = directory_path_ + args;

            if(command_name == "delete")
            {
                // user requested server to delete file
                // propagates to other sessions
                this->client_requested_delete_(args, buffer);
                break;
            }
            else if(command_name == "clist")
            {
                // client listing command probably failed
                this->client_sent_clist_(buffer, args);
                break;
            }
            else if(command_name == "download" || command_name == "adownload")
            {
                // user is requesting a file download to
                // keep in a non synchronized folder
                // send as "aupload"
                this->client_requested_download_(args, 'a');
                break;
            }
            else
            {
                // client sent malformed command
                this->malformed_command_(command_name);
                break;
            }
        }
        case 3:
        {
            std::string command_name = received_buffer[0];
            std::string args = received_buffer[1];
            std::string checksum = received_buffer[2];
            
            std::string local_file_path = directory_path_ + args;

            if(command_name == "sdownload")
            {
                // user is sending some file
                this->client_sent_sdownload_(args, buffer, checksum);
                break;
            }
            else if(command_name == "upload")
            {
                // user sent back server file upload request
                this->client_sent_supload_(args, buffer, checksum);
                break;
            }
            else
            {
                // malformed command
                this->malformed_command_(command_name);
                break;
            }
        }
        default:
        {
            // malformed command
            std::string command_name = received_buffer[0];
            this->malformed_command_(command_name);
            break;
        }
    }
}

bool ClientSession::is_sync()
{
    // checks if the synchronization routine was enabled by the session end
    // by default this starts off as disabled untill the session requests it
    return running_sync_.load();
}

void ClientSession::start_sync()
{
    // starts up the synchronization routine
    // this should be called uppon the server receiving a sync request
    // which then requests the files currently being hosted by the session
    // to then update it with the most recent files, or download the files
    // it currently does not have

    if(running_sync_.load())
    {
        aprint("Synchronization routine is already running!", 3);
        return;
    }

    // TODO: sends slist to client

    running_sync_.store(true);
}

void ClientSession::stop_sync()
{
    // stops the synchronization routine, deleting any
    // temporary download files on server
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

            if(running_sender_.load() == false)
            {
                
                return;
            }

            if(!sender_buffer_.empty())
            {
                // sends using previoulsy set callback method
                this->send_packet_(sender_buffer_.front(), socket_fd_);

                sender_buffer_.erase(sender_buffer_.begin());
            }

        }
        catch(const std::exception& e)
        {
            std::string output = "Exception occured while running send: " + std::string(e.what());
            raise(output, 2);
        }

        send_cv_.notify_one();
    }
}

void ClientSession::send_ping()
{
    // server is measuring connection ping with client
    // mounts a response packet and adds to send queue
    packet ping_packet;
    std::string command_string = "ping";
    strcharray(command_string, ping_packet.command, sizeof(ping_packet.command));
    
    // adds to sender buffer
    {
        std::unique_lock<std::mutex> lock(send_mtx_);
        sender_buffer_.push_back(ping_packet);
    }
    ping_start_ = std::chrono::high_resolution_clock::now();
    send_cv_.notify_one();
}

void ClientSession::disconnect(std::string reason)
{
    packet exit_packet;
    std::string command_response = "exit";
    strcharray(command_response, exit_packet.command, sizeof(exit_packet.command));
    std::string args_response = reason;
    exit_packet.payload = new char[args_response.size()+1];
    strcharray(args_response, exit_packet.payload, args_response.size() + 1);
    exit_packet.payload_size = args_response.size();

    // adds to sender buffer
    {
        std::unique_lock<std::mutex> lock(send_mtx_);
        sender_buffer_.push_back(exit_packet);
    }
    send_cv_.notify_one();

    running_receiver_.store(false);
    running_sender_.store(false);
    send_cv_.notify_one();

    // close(socket_fd_);
}

void ClientSession::send_packet_(const packet& p, int sockfd, int timeout)
{
    send_callback_(p, sockfd, timeout);
}

void ClientSession::receive_packet_(packet* p, int sockfd, int timeout)
{
    receive_callback_(p, socket_fd_, timeout);
}
