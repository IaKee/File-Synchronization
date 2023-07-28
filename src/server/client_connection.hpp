#pragma once

#include <string>
#include <thread>
#include <list>

namespace client_connection
{
    class ClientConnections
    {
        public:
            ClientConnections(std::string username);
            
            std::string get_username();
            void nuke();  // disconnect all sessions

            int get_active_sessions();
            int get_available_space();
            int get_used_space();

            void add_connection();
            void remove_connection();

            bool has_available_space();
            bool has_current_files();
            bool
            ClientConnection* find_connection(int sock_id);
            void disconnect_inactive();

        private:
            std::string username_;
            std::list<ClientConnection> connections_;

            int max_sessions_;
            int last_activity_;

            // benchmark
            int bytes_sent_;
            int bytes_recieved_;

    };
    class ClientConnection
    {
        // client connection instance to server
        public:
            ClientConnection(int sock, std::string machine_name);

            int get_id();
            int get_ip();
            std::string get_machine();
            std::string get_dir_path();
            int ping();

        private:
            // identifiers
            int sock_id_;  // connection identifier
            std::string machine_name_;
            std::string address_;

            std::string sync_dir_;

            // other
            std::list<std::string> command_history_;
            int last_activity_;
            int bytes_sent_;
            int bytes_recieved_;
            bool is_active_;
            int connection_start_time_;




    };
}