#pragma once

// c++
#include <string>
#include <vector>
#include <thread>
#include <functional>
#include <condition_variable>

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
                std::function<void(int sock_fd, char* buffer, std::size_t buffer_size)> send_callback,
                std::function<void(int sock_fd, char* buffer, std::size_t buffer_size)> recieve_callback,
                std::function<void(std::string message, std::string type)> broadcast_user_callback,
                std::function<void(int sock_fd)> remove_callback);
            
            // connection identifiers
            int get_socket_fd();
            std::string get_username();
            std::string get_address();
            std::string get_machine_name();

            // network
            void send(char* buffer, std::size_t buffer_size);
            void recieve(char* buffer, std::size_t buffer_size);
            int ping();
            void disconnect(std::string reason = "");

            // main handler
            void start_handler();
            void stop_handler();
            void session_handler_loop();
        private:
            // identifiers
            int socket_fd_;  // connection identifier
            std::string username_;
            std::string address_;
            std::string machine_;
            std::string directory_path_;

            // multithread & synchronization
            bool handler_started_;
            std::thread handler_th_;
            std::mutex send_mtx_;
            std::mutex recieve_mtx_;

            // callbacks
            std::function<void(int sock_fd, char* buffer, std::size_t buffer_size)> send_callback_;
            std::function<void(int sock_fd, char* buffer, std::size_t buffer_size)> recieve_callback_;
            std::function<void(std::string message, std::string type)> broadcast_user_callback_;
            std::function<void(int sock_fd)> remove_callback_;
    };
    
    class User
    {
        public:
            User(std::string username, std::string home_dir);
            
            // identification
            std::string get_username();

            // session control
            void add_session(ClientSession new_session);
            void remove_session(int sock_fd, std::string reason = "");
            ClientSession* get_session(int sock_fd);
            void nuke();  // disconnect all sessions
            void broadcast(char* buffer, std::size_t buffer_size);

            // other
            int get_session_limit();
            bool has_available_space();
            bool has_current_files();
            
            // overseer control
            void start_overseer();
            void stop_overseer();
            void overseer_loop();
        private:
            // constants
            std::string home_dir_path_;
            std::string user_dir_path_;

            // identifiers
            std::string username_;
            
            // multithread & synchronization
            std::thread overseer_th_;
            int max_timeout_;
            bool overseer_started_;
            bool overseer_stop_requested_;

            //InotifyWatcher inotifyWatcher_;

            // user attributes
            int max_sessions_;  // maximum number of connections allowed for this user
            int max_space_; // the amount of disk space this user may use
            const int max_sessions_default_ = 2;
            const int max_space_default_ = 209715200;

            // main user manager vector
            std::vector<ClientSession> sessions_;
    };

    class UserGroup
    {
        public:
            UserGroup(
                std::function<void(char* buffer, std::size_t buffer_size, int sock_fd, int timeout)> send_callback,
                std::function<void(char* buffer, std::size_t buffer_size, int sock_fd, int timeout)> recieve_callback);

            // group control
            User* get_user(std::string username);
            void load_user(std::string username);
            void unload_user(std::string username);
            void delete_user(std::string username);
            void global_broadcast(char* buffer, std::size_t buffer_size);

            int get_active_users_count();

        private:
            std::thread director_th_;
            std::string sync_dir_ = "./sync_dir_server";
            std::vector<User> users_;

            // callbacks
            std::function<void(char* buffer, std::size_t buffer_size, int sock_fd, int timeout)> send_callback_;
            std::function<void(char* buffer, std::size_t buffer_size, int sock_fd, int timeout)> recieve_callback_;
    };
}