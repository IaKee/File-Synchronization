# pragma once

// c++
#include <iostream>
#include <string>
#include <thread>

// connection
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "../include/common/connection.hpp"

// locals
#include "client_connection.hpp"
#include "../include/common/file_manager.hpp"
#include "../include/common/json.hpp"
#include "../include/common/user_interface.hpp"

namespace server
{
    class Server
    {
        public:

            // synchronization
            std::mutex mutex_;
            std::condition_variable cv_;
            std::string command_buffer_;
            std::list<std::string> sanitized_commands_;

            // init & destroy
            Server();
            ~Server();

            // methods
            void start();
            void stop();
            void close();
            void handleClient(int clientSocket);
            void process_input();
            void main_loop();

        private:
            // runtime control
            bool running_;
            std::atomic<bool> stop_requested_;

            // connection
            connection::Connection conn_;
            std::string client_default_path_;

            // I/O
            user_interface::UserInterface S_UI_;
            file_manager::FileManager f_manager_;
            std::string config_dir_ = "./config";
            std::string config_file_path_ = "./config/server.json";
            std::string sync_dir_ = "./sync_dir";
            std::thread accept_th_;

            // synchronization
            int signal_pipe_[2];

            // clients
            std::list<client_connection::ClientConnection> clients_;

            // methods
            void read_configs_();
            void accept_connections_();
    };
}