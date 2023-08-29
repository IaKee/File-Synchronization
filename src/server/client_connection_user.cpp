// c++
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
#include "../include/common/json.hpp"
#include "../include/common/utils.hpp"

using namespace client_connection;
namespace fs = std::filesystem;
using json = nlohmann::json;

User::User(
    std::string username, 
    std::string home_dir)
    :   home_dir_path_(home_dir),
        user_dir_path_(home_dir + "/" + username),
        username_(username),
        overseer_running_(false),
        overseer_started_(false),
        overseer_stop_requested_(false),
        max_sessions_(max_sessions_default_)
{
    // checks if user had a folder on the server
    if(!fs::exists(user_dir_path_))
    {
        async_utils::async_print("\t[USER MANAGER] Creating folder for user " + username + "...");
        if(!create_directory(user_dir_path_))
        {
            throw std::runtime_error("[USER MANAGER] Could not create folder for user " + username);
        }
    }

    // after loading user, starts up overseer thread to process user events
    start_overseer();
    async_utils::async_print("\t[USER MANAGER] Overseer initialized for user \"" + username_ + "\"...");
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

void User::broadcast(char* buffer, std::size_t buffer_size)
{
    // sends a message to all user sessions
    async_utils::async_print("[USER MANAGER] Broadcasting user " + username_);

    for(client_connection::ClientSession* session : sessions_)
    {
        try
        {
            session->send(buffer, buffer_size);
        }
        catch(const std::exception& e)
        {
            throw std::runtime_error("[USER MANAGER] An exception happened while sending buffer to session:" + std::string(e.what()));
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
    async_utils::async_print("\t[USER MANAGER] Initializing overseer for user \"" + username_ + "\"...");
    overseer_running_.store(true);
    overseer_started_.store(true);
    overseer_th_ = std::thread(
        [this]()
        {
            overseer_loop();
        });
    //inotifyWatcher_.startWatching();
}

void User::stop_overseer()
{
    overseer_running_.store(false);
    overseer_stop_requested_.store(true);
    if(overseer_started_)
    {
        overseer_th_.join();
    }
    else
    {
        throw std::runtime_error("[USER MANAGER] Overseer stop requested without ever running!");
    }
}

void User::overseer_loop()
{
    while(overseer_running_.load())
    {
        sleep(1);
        async_utils::async_print("\t[USER MANAGER] looping overseer for " + username_);
    }
}

void handleFileEvent(){
    async_utils::async_print("File event detected on client: " << event.filename);

}