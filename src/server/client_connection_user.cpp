// c++
#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#include <exception>
#include <stdexcept>
#include <filesystem>
#include <unistd.h>

// multithreading & synchronization
#include <thread>
#include <mutex>
#include <condition_variable>

// locals
#include "client_connection.hpp"
#include "../include/common/utils.hpp"
#include "../include/common/async_cout.hpp"

using namespace async_cout;
using namespace client_connection;
namespace fs = std::filesystem;

User::User(
    std::string username, 
    std::string home_dir)
    :   home_dir_path_(home_dir),
        user_dir_path_(home_dir + "/" + username),
        username_(username),
        overseer_running_(false),
        max_sessions_(max_sessions_default_)
{
    // checks if user had a folder on the server
    if(!fs::exists(user_dir_path_))
    {
        aprint("\t[USER MANAGER] Creating folder for user " + username + "...");
        if(!create_directory(user_dir_path_))
        {
            throw std::runtime_error("[USER MANAGER] Could not create folder for user " + username);
        }
    }

    // after loading user, starts up overseer thread to process user events
    start_overseer();
    aprint("\t[USER MANAGER] Overseer initialized for user \"" + username_ + "\"...");
}

std::string User::get_username()
{
    return username_;
}

void User::add_session(client_connection::ClientSession* new_session)
{
    // checks the maximum connection number
    if(sessions_.size() <= max_sessions_)
    {
        sessions_.push_back(new_session);
    }
    else
    {
        throw std::runtime_error("[USER MANAGER] Session limit reached for this user!");
    }

}

void User::remove_session(int sock_fd, std::string reason)
{
    // checks if machine is already connected,
    // if it is under the maximum connection number
    if(get_session(sock_fd) != nullptr)
    {
        throw std::runtime_error("[USER MANAGER] No session exists under given socket descriptor!");
    }
    else
    {
        // iterates through sessions
        auto it = sessions_.begin();
        while(it != sessions_.end()) 
        {
            if ((*it)->get_socket_fd() == sock_fd) 
            {   
                // ends session properly previous to deleting it
                (*it)->disconnect(reason);

                // after disconnecting, removes it from the list
                it = sessions_.erase(it);
                return;
            }
            else
            {
                ++it;
            }
        }
    }
    throw std::runtime_error("[USER MANAGER] Could not find given session to remove!");
}

client_connection::ClientSession* User::get_session(int sock_fd)
{
    // iterates though client list
    for(client_connection::ClientSession* session : sessions_)
    {
        if(session->get_socket_fd() == sock_fd)
        {
            return session;
        }
    }
    return nullptr;
}

void User::nuke()
{
    // disconnects all active sessions
    for(client_connection::ClientSession* session : sessions_)
    {
        session->disconnect("Nuked!");
    }
}

void User::broadcast_other_sessions(int caller_sockfd, packet& p)
{
    // sends a message to another user sessions
    // excluding the one that actually sent it

    // this prints for every packet, remove for better performance
    std::string output = "\t[USER MANAGER] Broadcasting packet on user " + username_;
    output += " from session " + std::to_string(caller_sockfd) + ".";
    aprint(output);

    for(client_connection::ClientSession* session : sessions_)
    {
        try
        {
            // does not send back to the sender
            if(session->get_socket_fd() != caller_sockfd)
            {
                session->add_packet_from_broadcast(p);
            }
        }
        catch(const std::exception& e)
        {
            std::string output = "\t[USER MANAGER] An exception happened ";
            output += "while sending buffer to session " + session->get_socket_fd();
            output += "! Errors caught: " + std::string(e.what());
            aprint(output);
        }     
    }
}

void User::broadcast(packet& p)
{
    // sends a packet to all sessions
    for(client_connection::ClientSession* session : sessions_)
    {
        try
        {
            session->add_packet_from_broadcast(p);
        }
        catch(const std::exception& e)
        {
            std::string output = "\t[USER MANAGER] An exception happened ";
            output += "while sending buffer to session " + session->get_socket_fd();
            output += "! Errors caught: " + std::string(e.what());
            aprint(output);
        }     
    }
}

int User::get_active_session_count()
{
    return sessions_.size();
}

bool User::has_current_files()
{
    // TODO: check if user is up to date using inotify
    return false;
}

std::string User::get_home_directory()
{
    return home_dir_path_;
}

void User::start_overseer()
{
    aprint("\t[USER MANAGER] Initializing overseer for user \"" + username_ + "\"...");
    overseer_running_.store(true);
    overseer_th_ = std::thread(
        [this]()
        {
            overseer_loop();
        });
}

void User::stop_overseer()
{
    overseer_running_.store(false);
    if(overseer_th_.joinable())
    {
        overseer_th_.join();
    }
}

void User::overseer_loop()
{
    while(overseer_running_.load() == true)
    {
        // every minute, checks if sessions are alive
        std::this_thread::sleep_for(std::chrono::seconds(60));

        for(client_connection::ClientSession* session : sessions_)
        {
      
            std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
            std::chrono::minutes minutes_passed = std::chrono::duration_cast<std::chrono::minutes>(now - session->get_last_ping());

            if(minutes_passed.count() > 5)
            {
                // 5 minutes passed, nukes session
                session->disconnect("kicked due to inactivity");
            }
            else
            {
                // tries renew timer by pinging session
                session->send_ping();
            }
        }
    }
}