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
            int accept_connection();
            void close_socket();
            string get_host_by_name(const string& host_name) ;

        private:
            int sockfd;
    };
}