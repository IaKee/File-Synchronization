// c++
#include <string>
#include <chrono>
#include <ctime>
#include <exception>
#include <stdexcept>
#include <filesystem>

// multithreading & synchronization
#include <thread>
#include <mutex>
#include <condition_variable>

// locals
#include "client_connection.hpp"
#include "../include/common/json.hpp"
#include "../include/common/utils.hpp"
#include "../include/common/InotifyWatcher.hpp"

using namespace client_connection;
namespace fs = std::filesystem;
using json = nlohmann::json;

        /*inotifyWatcher_(user_dir_path_, std::bind(&User::handleFileEvent, this, std::placeholders::_1))*/
User::User(
    std::string username, 
    std::string home_dir,
    std::function<void(std::string username)> remove_callback)
    :   home_dir_path_(home_dir),
        user_dir_path_(home_dir + "\\" + username),
        username_(username),
        overseer_started_(false),
        overseer_stop_requested_(false)
{
    // tries to read previously saved user data
    // initializes a SessionHistory instance for each previous activity saved
    if(fs::exists(user_dir_path_))
    {
        // checks if user had a folder on the server
    }
    else
    {
        // notifies and creates folder
    }

    // after loading user, starts up overseer thread to process user events
    start_overseer();
}

std::string User::get_username()
{
    return username_;
}

void User::add_session(client_connection::ClientSession new_session)
{
    // checks the maximum connection number
    if(sessions_.size() < max_sessions_)
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
            if (it->get_socket_fd() == sock_fd) 
            {   
                // ends session properly previous to deleting it
                it->disconnect(reason);

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
    for(client_connection::ClientSession& session : sessions_)
    {
        if(session.get_socket_fd() == sock_fd)
        {
            return &session;
        }
    }
    return nullptr;
}

void User::nuke()
{
    // disconnects all active sessions
    for(client_connection::ClientSession& session : sessions_)
    {
        session.disconnect("Nuked!");
    }
}

void User::broadcast(char* buffer, std::size_t buffer_size)
{
    // sends a message to all user sessions
    async_utils::async_print("[USER MANAGER] Broadcasting user " + username_);

    for(client_connection::ClientSession& session : sessions_)
    {
        try
        {
            session.send(buffer, buffer_size);
        }
        catch(const std::exception& e)
        {
            throw std::runtime_error("[USER MANAGER] An exception happened while sending buffer to session:" + std::string(e.what()));
        }     
    }
}

int User::get_cloud_available_space()
{
    {
        std::unique_lock<std::mutex> lock(bench_mtx_);
        bench_cv_.wait(lock, [this]() {return bench_ready_.load();});
    }

    bench_ready_.store(false);

    int available_space = max_space_ - get_folder_space(user_dir_path_, "free");

    {
        bench_ready_.store(true);
        bench_cv_.notify_one();
    }

    return available_space;
}

int User::get_cloud_used_space()
{
    {
        std::unique_lock<std::mutex> lock(bench_mtx_);
        bench_cv_.wait(lock, [this]() {return bench_ready_.load();});
    }

    bench_ready_.store(false);

    int used_space = get_folder_space(user_dir_path_, "used");

    {
        bench_ready_.store(true);
        bench_cv_.notify_one();
    }

    return used_space;
}

int User::get_active_session_count()
{
    return sessions_.size();
}

int User::get_session_limit()
{
    return max_sessions_;
}

bool User::has_available_space()
{
    {
        std::unique_lock<std::mutex> lock(bench_mtx_);
        bench_cv_.wait(lock, [this]() {return bench_ready_.load();});
    }

    bench_ready_.store(false);
    bool has_space = (max_space_ - get_cloud_available_space() > 0);

    {
        bench_ready_.store(true);
        bench_cv_.notify_one();
    }

    if(has_space)
    {
        return true;
    }
    return false;
}

bool User::has_current_files()
{
    // TODO: check if user is up to date using inotify
    return false;
}

void User::start_overseer()
{
    overseer_th_ = std::thread(&User::overseer_loop_, this);
    overseer_started_ = true;
    //inotifyWatcher_.startWatching();
}

void User::stop_overseer()
{
    if(overseer_started_)
    {
        overseer_th_.join();
    }
    else
    {
        throw std::runtime_error("Overseer stop requested without ever running!");
    }
}

void User::overseer_loop_()
{

}

void handleFileEvent(){
    //std::cout << "File event detected on client: " << event.filename << std::endl;
}