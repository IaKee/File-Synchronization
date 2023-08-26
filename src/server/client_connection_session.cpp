// c++
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <chrono>
#include <exception>
#include <stdexcept>
#include <filesystem>
#include <vector>

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
    std::function<void(int sock_fd, char* buffer, std::size_t buffer_size)> net_send,
    std::function<void(int sock_fd, char* buffer, std::size_t buffer_size)> net_recv,
    std::function<void(std::string message, std::string type)> broadcast_server_callback,
    std::function<void(std::string message, std::string type)> broadcast_client_callback,
    std::function<void(int sock_fd)> remove_callback)
    :   socket_fd_(sock_fd),
        username_(username),
        machine_(machine_name),
        net_send_(net_send),
        net_recv_(net_recv),
        broadcast_server_callback_(broadcast_server_callback),
        broadcast_client_callback_(broadcast_client_callback),
        remove_callback_(remove_callback),
        start_time_(get_time()),
        last_activity_(get_time()),
        bytes_sent_(0),
        bytes_recieved_(0),
        is_active_(true),
        handler_started_(false)
{
    // initializes session sending a quick ping to confirm connection
    last_ping_ = ping();
    
    std::string message = (username_ + "@" + machine_ + ":" + std::to_string(sock_fd) + 
        " connection stablished with a ping of " + std::to_string(last_ping_) + "ms.");
    broadcast_server_callback_(message, "message");
    broadcast_client_callback_(message, "message");
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

void ClientSession::start_handler()
{

}

void ClientSession::stop_handler()
{

}

void ClientSession::send(char* buffer, std::size_t buffer_size)
{
    // sends a buffer along with its own socket descriptor
    net_send_(socket_fd_, buffer, buffer_size);
}

void ClientSession::recieve(char* buffer, std::size_t buffer_size)
{
    // waits for a given buffer along with its own socket descriptor
    net_recv_(socket_fd_, buffer, buffer_size);
}

int ClientSession::ping()
{
    auto start_time = std::chrono::high_resolution_clock::now();

    char ping_buffer[] = "ping";
    send(ping_buffer, sizeof(ping_buffer));

    char recv_buffer[1024];
    recieve(recv_buffer, sizeof(recv_buffer));
    
    if(std::strcmp(recv_buffer, "pong") == 0)
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        {
            std::unique_lock<std::mutex> lock(bench_mtx_);
            bench_cv_.wait(lock, [this]() {return bench_ready_.load() || is_active_.load();});
        }

        if(!is_active_.load())
        {
            bench_cv_.notify_one();
            return -1;
        }

        bench_ready_.store(false);
        last_ping_ = static_cast<int>(duration_ms);
        int last_ping_local = last_ping_;
        last_activity_ = get_time();
        bytes_sent_ += 4;
        bytes_recieved_ += 4;
        is_active_ = true; 

        // frees bench attributes
        {
            bench_ready_.store(true);
            bench_cv_.notify_one();
        }

        return last_ping_local;
    }
    else
    {
        throw std::runtime_error("Invalid response! Could not ping client!");
    }
    return -1;
}

void ClientSession::disconnect(std::string reason)
{
    char disconnect_buffer[1024];
    std::string disconnect_string = "logout|" + reason;
    strcpy(disconnect_buffer, disconnect_string.c_str());

    // informs the user about the disconnection
    send(disconnect_buffer, sizeof(disconnect_buffer));

    stop_handler();

    // terminates session handler thread
    if(handler_th_.joinable())
    {
        handler_th_.join();
    }

    // removes session from user manager vector
    remove_callback_(socket_fd_);
}