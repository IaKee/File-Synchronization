
// Connection.hpp
#pragma once

// C++
#include <unistd.h>
#include <cstring>
#include <string>
#include <stdexcept>

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
            Connection();
            bool create_socket();
            bool create_server();
            void connect_to_server(const std::string& ipAddress, int port);
            void handle_connection();
            std::string get_host_by_name(const std::string& host_name);
            int accept_connection();
            void close_socket();
            void set_port(int port);
            std::string get_address();
            int get_port();


        private:
            int sockfd_;
            int port_;
            int backlog_;
            int server_fd_;
            std::string address_;
            
    };
}