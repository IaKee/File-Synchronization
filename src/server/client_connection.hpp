#pragma once

// c++
#include <string>
#include <vector>
#include <thread>
#include <functional>
#include <list>

// synchronization
#include <atomic>
#include <mutex>
#include <condition_variable>

// locals
#include "../include/common/InotifyWatcher.hpp"

namespace client_connection
{
    class ClientSession
    {
        // client connection instance to server
        public:
            ClientSession(
                int sock_fd,
                std::string username, 
                std::string machine_name,
                std::string directory_path,
                std::function<void(char* buffer, std::size_t buffer_size, int sock_fd, int timeout)> send_callback,
                std::function<void(char* buffer, std::size_t buffer_size, int sock_fd, int timeout)> recieve_callback,
                std::function<void(char* buffer, std::size_t buffer_size)> broadcast_user_callback);
            
            ~ClientSession();

            // connection identifiers
            int get_socket_fd();
            std::string get_username();
            std::string get_address();
            std::string get_machine_name();

            // network
            void send(char* buffer, std::size_t buffer_size, int timeout = -1);
            void recieve(char* buffer, std::size_t buffer_size, int timeout = -1);
            void send_ping();
            void disconnect(std::string reason = "");

            // main handler
            void start_handler();
            void stop_handler();
            void session_handler_loop();
            
            // recieve handler
            void start_receiver();
            void stop_receiver();
            void session_receiver_loop();

            // send handler
            void start_sender();
            void stop_sender();
            void session_sender_loop();
        private:
            // identifiers
            int socket_fd_;  // connection identifier
            std::string username_;
            std::string address_;
            std::string machine_;
            std::string directory_path_;

            // multithread & synchronization
            std::atomic<bool> initializing_;

            std::mutex send_mtx_;
            std::condition_variable send_cv_;
            std::atomic<bool> running_sender_;
            std::thread sender_th_;
            std::list<std::pair<char*, std::size_t>> sender_buffer_;

            std::atomic<bool> running_receiver_;
            std::thread receiver_th_;
            std::mutex recieve_mtx_;
            std::size_t expected_buffer_size = 1024;
            std::list<std::pair<char*, std::size_t>> receiver_buffer_;
            const char EOF_MARKER = 0xFF;

            // other
            std::atomic<int> timeout_sec = 5;
            std::chrono::high_resolution_clock::time_point ping_start_;
            std::chrono::high_resolution_clock::time_point last_ping_;
            
            // callbacks
            std::function<void(char* buffer, std::size_t buffer_size, int sock_fd, int timeout)> send_callback_;
            std::function<void(char* buffer, std::size_t buffer_size, int sock_fd, int timeout)> recieve_callback_;
            std::function<void(char* buffer, std::size_t buffer_size)> broadcast_user_callback_;
            std::function<int()> get_user_count_callback_;
    };
    
    class User
    {
        private:
            // constants
            std::string home_dir_path_;
            std::string user_dir_path_;

            // identifiers
            std::string username_;
            
            // multithread & synchronization
            std::thread overseer_th_;
            int max_timeout_;
            std::atomic<bool> overseer_running_;
            std::atomic<bool> overseer_started_;
            std::atomic<bool> overseer_stop_requested_;
            std::mutex broadcast_mtx_;

            // user attributes
            int max_sessions_;  // maximum number of connections allowed for this user
            const int max_sessions_default_ = 2;

            // main user manager vector
            std::vector<ClientSession*> sessions_;
        public:
            User(std::string username, std::string home_dir);
            
            // identification
            std::string get_username();

            // session control
            void add_session(ClientSession* new_session);
            void remove_session(int sock_fd, std::string reason = "");
            ClientSession* get_session(int sock_fd);
            void nuke();  // disconnect all sessions
            void broadcast(char* buffer, std::size_t buffer_size);

            // other
            bool has_current_files();
            int get_active_session_count();
            std::string get_home_directory();
            
            // overseer control
            void start_overseer();
            void stop_overseer();
            void overseer_loop();
            void handleFileEvent();
    };

    class UserGroup
    {
        public:
            UserGroup(
                std::function<void(char* buffer, std::size_t buffer_size, int sock_fd, int timeout)> send_callback,
                std::function<void(char* buffer, std::size_t buffer_size, int sock_fd, int timeout)> recieve_callback);

            // group control
            std::list<std::string> list_users();
            User* get_user(std::string username);
            void load_user(std::string username);
            void unload_user(std::string username);
            void global_broadcast(char* buffer, std::size_t buffer_size);

            int get_active_users_count();

        private:
            std::thread director_th_;
            std::string sync_dir_ = "./sync_dir_server";

            // callbacks
            std::function<void(char* buffer, std::size_t buffer_size, int sock_fd, int timeout)> send_callback_;
            std::function<void(char* buffer, std::size_t buffer_size, int sock_fd, int timeout)> recieve_callback_;
            
            std::vector<User*> users_;
    };
}