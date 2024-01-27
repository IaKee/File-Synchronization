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
#include "../client_connection.hpp"
#include "../../../../common/include/utils.hpp"
#include "../../../../common/include/asyncio/async_cout.hpp"

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
        aprint("Creating folder for user " + username + "...", 4);
        if(!create_directory(user_dir_path_))
        {
            raise("Could not create folder for user " + username, 4);
        }
    }

    // after loading user, starts up overseer thread to process user events
    start_overseer();
    aprint("Overseer initialized for user \"" + username_ + "\"...", 4);
}

std::string User::get_username()
{
    return username_;
}

void User::add_session(client_connection::ClientSession* new_session)
{
    // checks the maximum connection number
    if(sessions_.size() < max_sessions_default_)
    {
        sessions_.push_back(new_session);
    }
    else
    {
        aprint(std::to_string(sessions_.size()), 0);
        aprint(std::to_string(max_sessions_default_), 0);
        aprint(std::to_string(max_sessions_), 0);
        raise("Session limit reached for this user!", 4);
    }

}

void User::remove_session(int sock_fd, std::string reason)
{
    // checks if machine is already connected,
    // if it is under the maximum connection number
    if(get_session(sock_fd) == nullptr)
    {
        raise("No session exists under given socket descriptor!", 4);
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
    raise("Could not find given session to remove!", 4);
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

std::vector<ClientSession*> User::get_sessions()
{
    return sessions_;
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
    std::string output = "Broadcasting packet on user " + username_;
    output += " from session " + std::to_string(caller_sockfd) + ".";
    aprint(output, 4);

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
            std::string output = "An exception happened ";
            output += "while sending buffer to session " + session->get_socket_fd();
            output += "! Errors caught: " + std::string(e.what());
            aprint(output, 4);
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
            std::string output = "An exception happened ";
            output += "while sending buffer to session " + session->get_socket_fd();
            output += "! Errors caught: " + std::string(e.what());
            aprint(output, 4);
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
    aprint("Initializing overseer for user \"" + username_ + "\"...", 4);
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
        std::this_thread::sleep_for(std::chrono::seconds(1));

        for(client_connection::ClientSession* session : sessions_)
        {
      
            std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> seconds_passed = std::chrono::duration_cast<std::chrono::duration<double>>(now - session->get_last_ping());
            printf("%f\n", seconds_passed.count());

            session->send_ping();

            if(seconds_passed.count() > 3.0f)
            {
                // 5 seconds passed, nukes session
                session->disconnect("Kicked due to inactivity");
                printf("Tomou dc");
            }
            // else
            // {
            //     // tries renew timer by pinging session
            //     session->send_ping();
            // }
        }
    }
}