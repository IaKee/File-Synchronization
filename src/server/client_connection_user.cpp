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

using namespace client_connection;
namespace fs = std::filesystem;
using json = nlohmann::json;

User::User(
    std::string username, 
    std::string home_dir,
    std::function<void(int sock_fd, char* buffer, std::size_t buffer_size)> net_send,
    std::function<void(int sock_fd, char* buffer, std::size_t buffer_size)> net_recv,
    std::function<void(std::string message, std::string type)> broadcast_server_callback,
    std::function<void(std::string message, std::string type)> broadcast_client_callback,
    std::function<void(int sock_fd, std::string reason)> disconnect_callback)
    :   home_dir_path_(home_dir),
        user_dir_path_(home_dir + "\\" + username),
        username_(username),
        net_send_(net_send),
        net_recv_(net_recv),
        broadcast_server_callback_(broadcast_server_callback),
        broadcast_client_callback_(broadcast_client_callback),
        overseer_started_(false),
        overseer_stop_requested_(false)
{
    // tries to read previously saved user data
    // initializes a SessionHistory instance for each previous activity saved
    if(fs::exists(user_dir_path_))
    {
        // reads user data
        json save_data = get_json_contents(user_dir_path_);

        // initializes user
        crashed_ = save_data["crashed"];
        max_sessions_ = save_data["max_sessions"];  // maximum number of connections allowed for this user
        max_space_ = save_data["max_space"]; // the amount of disk space this user may use
        bytes_sent_ = save_data["bytes_sent"];
        bytes_recieved_ = save_data["bytes_recieved"];
        first_seen_ = save_data["first_seen"];
        last_seen_ = save_data["last_seen"];

        // restore info about previous client sessions
        json local_activity_history = save_data["previous_activity"];
        for(const auto& session_json : local_activity_history)
        {
            int sock_fd = session_json["socket_id"];
            std::string username = session_json["username"];
            std::string address = session_json["address"];
            std::string machine_name = session_json["machine_name"];
            std::string sync_dir_path = session_json["sync_dir"];
            std::vector<std::string> command_history = session_json["command_history"];
            std::time_t start_time = session_json["start_time"];
            std::time_t last_activity = session_json["last_activity"];
            int bytes_sent = session_json["bytes_sent"];
            int bytes_recieved = session_json["bytes_recieved"];
            int last_ping = session_json["last_ping"];
            
            // creates a SessionHistory instance
            SessionHistory previous_session(
                sock_fd,
                username,
                address,
                machine_name,
                sync_dir_path,
                command_history,
                start_time,
                last_activity,
                bytes_sent,
                bytes_recieved,
                last_ping);

            previous_activity_.push_back(previous_session);
        }
    }
    else
    {
        // user is new, or could not find user folder - initializes defaults
        max_sessions_ = max_sessions_default_;
        max_space_ = max_space_default_;
        crashed_ = false;
        bytes_sent_ = 0;
        bytes_recieved_ = 0;
        first_seen_ = get_time();
        last_seen_ = first_seen_;
        previous_activity_.clear();
    }

    // after loading user, starts up overseer thread to process user events
    start_overseer();
}

std::string User::get_username()
{
    return username_;
}

void User::add_session(int sock_fd, ClientSession* new_session)
{
    // checks if machine is already connected,
    // if it is under the maximum connection number
    if(get_session(sock_fd) != nullptr)
    {
        throw std::runtime_error("Session already exists with given socket id!");
    }

    sessions_.push_back(new_session);
}

void User::remove_session(int sock_fd, std::string reason = "")
{
    // checks if machine is already connected,
    // if it is under the maximum connection number
    if(get_session(sock_fd) != nullptr)
    {
        throw std::runtime_error("No session exists under given socket descriptor!");
    }
    else
    {
        // iterates through sessions
        for (auto it = sessions_.begin(); it != sessions_.end(); ++it) 
        {
            if ((*it)->get_socket_fd() == sock_fd) 
            {   
                // ends session properly previous to deleting it
                (*it)->disconnect(reason);

                // after disconnecting, removes it from the list
                sessions_.erase(it);
                delete *it;  // frees memory corresponding to the pointed element
            
                break; // Para parar de procurar após encontrar e remover
            }
        }

    }
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