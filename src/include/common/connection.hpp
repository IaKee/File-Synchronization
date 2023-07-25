
// Connection.hpp
#pragma once

// C++
#include <unistd.h>
#include <cstring>
#include <string>
#include <stdexcept>
#include <atomic>
#include <thread>

// other
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

namespace connection
{
    class Connection 
    {
        public:
            // init & destroy
            Connection();

            // synchronization
            std::atomic<bool> running_accept_;

            // methods
            bool create_socket();
            void create_server();
            void connect_to_server(const std::string& ipAddress, int port);
            //void handle_connection();
            std::string get_host_by_name(const std::string& host_name);
            void close_socket();

            // main methods
            void download_file(const std::string& filename, const std::string& username);
            void upload_file(const std::string& filename, const std::string& username);
            void start_accepting_connections();
            void stop_accepting_connections();

            // other methods
            void set_port(int port);
            int get_port();
            std::string get_address();
            void link_pipe(int (&pipe)[2]);

        private:
            int sockfd_;
            int port_;
            int backlog_;
            std::string host_address_;
        
            int (*signal_pipe_)[2];
    };
}