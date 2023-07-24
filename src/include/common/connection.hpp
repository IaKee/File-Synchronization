
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

// namespace
using std::string;


namespace connection
{
    class Connection 
    {
        public:
            Connection();
            bool create_socket();
            void connect_to_server(const string& ipAddress, int port);
            void close_socket();
            string get_host_by_name(const string& host_name);

            void set_port(int port);
            bool create_server();
            void close_server();
            void handle_connection();
            int accept_connection();

        private:
            int sockfd_;
            int port_;
            int backlog_;
            int server_fd_;
            
    };
}