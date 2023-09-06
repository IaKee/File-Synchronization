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
#include "../include/common/utils.hpp"

using namespace client_connection;
namespace fs = std::filesystem;

ClientSession::ClientSession(
    int sock_fd, 
    std::string username,
    std::string machine_name,
    std::string directory_path,
    std::function<void(const packet& p, int sockfd, int timeout)> send_callback,
    std::function<void(packet& p, int sockfd, int timeout)> receive_callback,
    std::function<void(int caller_sockfd, packet& p)> broadcast_user_callback)
    :   socket_fd_(sock_fd),
        username_(username),
        machine_(machine_name),
        directory_path_(directory_path),
        send_callback_(send_callback),
        receive_callback_(receive_callback),
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

std::string ClientSession::get_identifier()
{
    return "@" + username_ + " (Session " + std::to_string(socket_fd_) + ")";
}

std::chrono::high_resolution_clock::time_point ClientSession::get_last_ping()
{
    return last_ping_;
}