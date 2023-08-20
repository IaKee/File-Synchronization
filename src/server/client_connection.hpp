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

namespace client_connection
{
    class SessionHistory
    {
        // client connection instance to server
        public:
            SessionHistory(
                int socket_fd,
                std::string username,
                std::string address,
                std::string machine,
                std::string sync_dir_path,
                std::vector<std::string> command_history,
                std::time_t start_time,
                std::time_t last_activity,
                int bytes_sent,
                int bytes_recieved,
                int last_ping);

            
            // identifiers
            int socket_fd;  // connection identifier
            std::string username;
            std::string address;
            std::string machine;
            std::string sync_dir_path;

            // benchmark
            std::vector<std::string> command_history;
            std::time_t start_time;
            std::time_t last_activity;
            int bytes_sent;
            int bytes_recieved;
            int last_ping;
    };
    class ClientSession
    {
        // client connection instance to server
        public:
            ClientSession(
                int sock_fd,
                std::string address,
                std::string username, 
                std::string machine_name,
                std::function<void(int sock_fd, char* buffer, std::size_t buffer_size)> net_send,
                std::function<void(int sock_fd, char* buffer, std::size_t buffer_size)> net_recv,
                std::function<void(std::string message, std::string type)> broadcast_server_callback,
                std::function<void(std::string message, std::string type)> broadcast_client_callback,
                std::function<void(int sock_fd, std::string reason)> disconnect_callback);

            // connection identifiers
            int get_socket_fd();
            std::string get_username();
            std::string get_address();
            std::string get_machine_name();

            // main handler
            void start_handler();
            void stop_handler();

            // network
            void send(char* buffer, std::size_t buffer_size);
            void recieve(char* buffer, std::size_t buffer_size);
            int ping();
            void disconnect(std::string reason);

            // disk space management
            std::string get_local_dir_path();

            // benchmark
            std::time_t get_start_time();  // check connection start time
            std::time_t get_last_activity();
            int get_bytes_transfered();

        private:
            // identifiers
            int socket_fd_;  // connection identifier
            std::string username_;
            std::string address_;
            std::string machine_;
            std::string sync_dir_path_;

            // multithread & synchronization
            bool handler_started_;
            std::thread handler_th_;
            std::atomic<bool> bench_ready_;
            std::mutex bench_mtx_;
            std::condition_variable bench_cv_;

            // benchmark
            std::vector<std::string> command_history_;
            std::time_t start_time_;
            std::time_t last_activity_;
            int bytes_sent_;
            int bytes_recieved_;
            int last_ping_;
            std::atomic<bool> is_active_;

            std::function<void(int sock_fd, char* buffer, std::size_t buffer_size)> net_send_;
            std::function<void(int sock_fd, char* buffer, std::size_t buffer_size)> net_recv_;
            std::function<void(std::string message, std::string type)> broadcast_server_callback_;
            std::function<void(std::string message, std::string type)> broadcast_client_callback_;
            std::function<void(int sock_fd, std::string reason)> disconnect_callback_;
            
    };
    
    class User
    {
        public:
            User(
                std::string username, 
                std::string home_dir,
                std::function<void(int sock_fd, char* buffer, std::size_t buffer_size)> net_send,
                std::function<void(int sock_fd, char* buffer, std::size_t buffer_size)> net_recv,
                std::function<void(std::string message, std::string type)> broadcast_server_callback,
                std::function<void(std::string message, std::string type)> broadcast_client_callback,
                std::function<void(int sock_fd, std::string reason)> disconnect_callback);
            
            // identification and connection flow
            std::string get_username();
            void add_session(int sock_fd, ClientSession* new_session);
            void remove_session(int sock_fd, std::string reason);
            ClientSession* get_session(int sock_fd);
            void nuke();  // disconnect all sessions

            // client session info
            int get_cloud_available_space();
            int get_cloud_used_space();
            int get_active_session_count();
            int get_session_limit();
            
            // other
            bool has_available_space();
            bool has_current_files();
            
            // overseer control
            void start_overseer();
            void stop_overseer();
        private:

            // constants
            std::string home_dir_path_;
            std::string user_dir_path_;

            // identifiers
            std::string username_;
            
            // multithread & synchronization
            std::vector<ClientSession*> sessions_;
            std::thread overseer_th_;
            int max_timeout_;
            bool overseer_started_;
            bool overseer_stop_requested_;
            void overseer_loop_();
            std::atomic<bool> bench_ready_;
            std::mutex bench_mtx_;
            std::condition_variable bench_cv_;

            // callbacks
            std::function<void(int sock_fd, char* buffer, std::size_t buffer_size)> net_send_;
            std::function<void(int sock_fd, char* buffer, std::size_t buffer_size)> net_recv_;
            std::function<void(std::string message, std::string type)> broadcast_server_callback_;
            std::function<void(std::string message, std::string type)> broadcast_client_callback_;
            std::function<void(int sock_fd, std::string reason)> disconnect_callback_;

            // user attributes
            int max_sessions_;  // maximum number of connections allowed for this user
            int max_space_; // the amount of disk space this user may use
            const int max_sessions_default_ = 2;
            const int max_space_default_ = 209715200;

            // benchmark
            bool crashed_;
            int bytes_sent_;
            int bytes_recieved_;
            std::time_t first_seen_;
            std::time_t last_seen_;
            std::vector<SessionHistory> previous_activity_;
    };

    class UserGroup
    {
        public:
            //UserGroup();

            // group control
            User* get_user(std::string username);
            void add_user(std::string username);
            void delete_user(std::string username);
            void load_user(std::string username);
            void unload_user(std::string username);  // does not delete user, just removes it from the loaded users

            int get_active_users_count();

        private:
            std::thread director_th_;

            std::vector<User*> users_;
    };
}