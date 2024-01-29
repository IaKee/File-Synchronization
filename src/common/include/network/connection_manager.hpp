
// Connection.hpp
#pragma once

// standard C++
#include <string>
#include <stdexcept>
#include <functional>
#include <vector>
#include <tuple>
#include <iostream>
#include <fstream>

// network related libraries
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

// multithreading & synchronization
#include <atomic>
#include <thread>
#include <condition_variable>

// locals
#include "packet.hpp"

using namespace utils_packet;

namespace connection
{
    void aprint(std::string content, int scope = 0, bool endl = true);
    void raise(std::string error, int scope = 0);
    
    class ConnectionManager 
    {
        public:
            ConnectionManager();
            ~ConnectionManager();

            // mutexes
            std::mutex receive_mtx_;
            std::mutex send_mtx_;

            // main attributes
            bool is_port_available(int port);
            int get_port();
            int get_sock_fd();
            int get_backlog();
            int get_send_timeout();
            int get_receive_timeout();
            std::string get_address();
            std::string get_host_by_name(std::string host_name);
            void set_port(int port);
            void set_address(std::string address);
            
            // socket control
            void create_socket();
            void close_socket();

            // main communication methods
            void send_data(char* buffer, std::size_t buffer_size, int sockfd = -1, int timeout = -1);
            void receive_data(char* buffer, std::size_t buffer_size, int sockfd = -1, int timeout = -1);
            void send_packet(const packet& p, int sockfd = -1, int timeout = -1);
            void receive_packet(packet* p, int sockfd = -1, int timeout = -1);
            
        private:
            // identifiers
            int sockfd_;
            int port_;
            std::string host_address_;

            // server list, first one is primary
            std::vector<std::pair<std::string, int>> backups_;

            // limiters
            int backlog_;
            int send_timeout_ = 2;
            int receive_timeout_ = 2;

            // runtime control 
            std::atomic<bool> running_send_;
            std::atomic<bool> running_receive_;

            // condition variables
            std::condition_variable receive_status_;
            std::condition_variable send_status_;
            
            // threads
            std::thread receive_th_;
            std::thread send_th_;

            // logging
            const bool console_log = true;
            const bool log_every_packet = false;
    };

    class ClientConnectionManager : public ConnectionManager
    {
        public:
            ClientConnectionManager();
            
            // addressing
            std::pair<std::string, int> main_server_address_;
            //std::vector<std::pair<std::string, int>> server_backups_address_;

            void connect_to_server(std::string ip_address, int port);
            int login(std::string username, std::string machine_name);
        private:
            std::atomic<bool> is_connected_;

            std::string username_;
            std::string machine_name_;

    };

    class ServerConnectionManager : public ClientConnectionManager
    {
        struct Server_info
        {
            std::pair<std::string, int> address;
            int key;
            bool cordinator;
            int socket;
        };

        public:
            ServerConnectionManager();
            ~ServerConnectionManager();

            // start main server functions - opens main socket for further connections
            void open_server();
            
            // main accept loop
            void start_accept_loop();
            void stop_accept_loop();
            void server_accept_loop(
                std::function<void(int, std::string, std::string)> connection_stablished_callback = nullptr,
                std::function<void(int, std::string, std::string)> connection_stablished_callback2 = nullptr);
            void add_servers();
            inline std::pair<std::string, int> get_first_address() { return this->server_backups_address_[0].address; }

            std::atomic<bool> coordinator;

        private:
            // multithreading & synchronization
            std::mutex accept_mtx_;
            std::condition_variable accept_status_;
            std::thread accept_th_;
            std::atomic<bool> running_accept_;

            // operating mode and backups
            
            int key;
            std::vector<Server_info> server_backups_address_;
    };
}