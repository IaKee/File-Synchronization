
// Connection.hpp
#pragma once

// C++
#include <unistd.h>
#include <cstring>
#include <string>
#include <stdexcept>
#include <functional>

// multithread & synchronization
#include <atomic>
#include <thread>
#include <condition_variable>

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
            ~Connection();

            // methods
            void create_socket();
            void create_server();
            void connect_to_server(const std::string& ip_addr, int port);
            void server_accept_loop(std::function<bool(int, std::string, std::string)> connection_stablished_callback = nullptr);
            void close_socket();
            void start_accepting_connections();
            void stop_accepting_connections();
            std::string get_host_by_name(const std::string& host_name);
            
            void send_data(char* buffer, std::size_t buffer_size, int sockfd = -1, int timeout = -1);
            void recieve_data(char* buffer, std::size_t buffer_size, int sockfd = -1, int timeout = -1);

            void upload_file(const std::string& filename, const std::string& username);
            void download_file(const std::string& filename, const std::string& username);
            
            void set_port(int port);
            int get_port();
            int get_sock_fd();
            std::string get_address();
            
        private:
            int sockfd_;
            int port_;
            int backlog_;
            int timeout_;  // timeout limit (seconds)
            std::string host_address_;

            // multithreading & synchronization
            std::mutex accept_mtx_;
            std::condition_variable accept_status_;
            std::thread accept_th_;
            std::atomic<bool> running_accept_;

            std::mutex recieve_mtx_;
            std::condition_variable recieve_status_;
            std::thread recieve_th_;
            std::atomic<bool> running_recieve_;

            std::mutex send_mtx_;
            std::condition_variable send_status_;
            std::thread send_th_;
            std::atomic<bool> running_send_;

            // runtime control 
    };
}