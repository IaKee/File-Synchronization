#pragma once

// c++
#include <string>
#include <thread>
#include <list>

// locals
#include "file_manager.hpp"

namespace client_connection
{
    class UserGroup
    {
        public:
            //UserGroup();

            // group control
            User* get_user(std::string username);  // gets connected user
            void login(std::string username);
            void logout(std::string username);

            int active_users_count();

        private:
            std::list<User> users_;
            file_manager::FileManager filesys_;

    };
    
    class User
    {
        public:
            User(std::string username);
            
            // identification and connection flow
            std::string get_username();
            void add_session();
            void remove_session(ClientConnection* client);
            void nuke();  // disconnect all sessions
            void disconnect_oldest(); // disconnect the oldest session
            void disconnect_inactive();

            // client session info
            int get_cloud_available_space();
            int get_cloud_used_space();
            
            // other
            bool has_available_space();
            bool has_current_files();
            bool is_new(); // checks if client is new or has connected to server before
            ClientConnection* get_connection(int sock_id);

        private:
            // identity
            std::string username_;
            std::list<ClientConnection> connections_;  // active connections

            // user attributes
            int max_sessions_;  // maximum number of connections allowed for this user
            int max_space_; // the amount of disk space this user may use

            // benchmark
            int bytes_sent_;
            int bytes_recieved_;
            std::time_t first_seen_;
            int last_seen_;
            std::list<ClientConnection> previous_activity_;


    };
    class ClientConnection
    {
        // client connection instance to server
        public:
            ClientConnection(int sock, std::string machine_name);

            // connection identifiers
            int get_socket_id();
            int get_ip();
            std::string get_machine_name();

            // disk space management
            std::string get_local_dir_path();
            int get_local_disk_free_space();
            int get_local_disk_used_space();

            // benchmark
            int ping();  // check response delay (and user activity)
            int get_start_time();  // check connection start time
            int get_last_activity();
            int get_bytes_transfered();
        private:
            // identifiers
            int socket_;  // connection identifier
            std::string username_;
            std::string machine_name_;
            std::string address_;

            std::string sync_dir_;

            // other
            std::list<std::string> command_history_;
            int start_time_;
            int last_activity_;
            int bytes_sent_;
            int bytes_recieved_;
            bool is_active_;
    };
}