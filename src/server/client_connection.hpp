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
#include <shared_mutex>

// locals
#include "../include/common/utils_packet.hpp"

using namespace utils_packet;

namespace client_connection
{
    // other
    void aprint(std::string content, int scope = 0, bool endl = true);
    void raise(std::string content, int scope = 0);

    class ClientSession
    {
        // client connection instance to server
        public:
            ClientSession(
                int sock_fd,
                std::string username, 
                std::string machine_name,
                std::string directory_path,
                std::function<void(const packet& p, int sockfd, int timeout)> send_callback_,
                std::function<void(packet& p, int sockfd, int timeout)> receive_callback_,
                std::function<void(int caller_sockfd, packet& p)> broadcast_user_callback);
            
            ~ClientSession();

            // connection identifiers
            int get_socket_fd();
            std::string get_username();
            std::string get_address();
            std::string get_machine_name();
            std::string get_identifier();
            std::chrono::high_resolution_clock::time_point get_last_ping();

            // network
            void send_ping();
            void disconnect(std::string reason = "");
            void add_packet_from_broadcast(packet& p);

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

            // internal buffers
            std::vector<packet> sender_buffer_;
            std::vector<packet> receiver_buffer_;
            std::unordered_map<std::string, std::shared_ptr<std::shared_mutex>> file_mtx_;

            // runtime control
            std::atomic<bool> initializing_;
            std::atomic<bool> running_sender_;
            std::atomic<bool> running_receiver_;

            // mutexes
            std::mutex send_mtx_;
            std::mutex recieve_mtx_;

            // condition variable
            std::condition_variable send_cv_;

            // threads
            std::thread sender_th_;
            std::thread receiver_th_;

            // timing
            std::chrono::high_resolution_clock::time_point ping_start_;
            std::chrono::high_resolution_clock::time_point last_ping_;
            
            // callbacks
            std::function<void(const packet& p, int sockfd, int timeout)> send_callback_;
            std::function<void(packet& p, int sockfd, int timeout)> receive_callback_;
            std::function<void(int caller_sockfd, packet& p)> broadcast_user_callback_;
            std::function<int()> get_user_count_callback_;
            
            // main commands
            void malformed_command_(std::string command);
            void client_requested_logout_();
            void client_requested_ping_();
            void client_responded_ping_();
            void client_requested_delete_(std::string args, packet buffer, std::string arg2 = "");
            void client_requested_slist_();
            void client_requested_flist_();
            void client_requested_adownload_(std::string args);
            void client_sent_clist_(packet buffer, std::string args = "");
            void client_sent_sdownload_(std::string args, packet buffer, std::string arg2 = "");
            void client_sent_supload_(std::string args, std::string arg2);
            std::string slist_();
            void receive_packet_(packet& p, int sockfd = -1, int timeout = -1);
            void send_packet_(const packet& p, int sockfd = -1, int timeout = -1);
    };
    
    class User
    {
        private:
            // identifiers
            std::string username_;

            // other attributes
            std::string home_dir_path_;
            std::string user_dir_path_;

            // threads
            std::thread overseer_th_;

            std::atomic<bool> overseer_running_;
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
            void broadcast_other_sessions(int caller_sockfd, packet& p);
            void broadcast(packet& p);

            // other
            bool has_current_files();
            int get_active_session_count();
            std::string get_home_directory();
            
            // overseer control
            void start_overseer();
            void stop_overseer();
            void overseer_loop();
    };

    class UserGroup
    {
        public:
            UserGroup(
                std::function<void(const packet& p, int sockfd, int timeout)> send_callback,
                std::function<void(packet& p, int sockfd, int timeout)> receive_callback);

            // group control
            std::list<std::string> list_users();
            User* get_user(std::string username);
            void load_user(std::string username);
            void unload_user(std::string username);
            void global_broadcast(packet& p);

            int get_active_users_count();

        private:
            std::thread director_th_;
            std::string sync_dir_ = "./sync_dir_server";

            // callbacks
            std::function<void(const packet& p, int sockfd, int timeout)> send_callback_;
            std::function<void(packet& p, int sockfd, int timeout)> receive_callback_;
            
            std::vector<User*> users_;
    };
}