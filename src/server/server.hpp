# pragma once

// c++
#include <iostream>
#include <string>
#include <thread>
#include <queue>

// connection
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "../include/common/connection.hpp"

// locals
#include "client_connection.hpp"
#include "../include/common/user_interface.hpp"
#include "../include/common/utils.hpp"

using json = nlohmann::json;

namespace server
{
    class Server
    {
        public:
            // synchronization
            std::mutex ui_mutex;
            std::condition_variable ui_cv;
            std::string ui_buffer;
            std::list<std::string> ui_sanitized_buffer;

            // init & destroy
            Server();
            ~Server();

            // methods
            void start();
            void stop();
            void close();
            void handle_new_session(int new_socket, std::string username, std::string machine);
            void process_input();
            void main_loop();

        
            // connection
            connection::Connection internet_manager;
        private:
            // runtime control
            bool running_;
            std::atomic<bool> stop_requested_;

            std::string client_default_path_;
            std::thread accept_th_;

            // interface
            user_interface::UserInterface S_UI_;

            // main settings
            std::string sync_dir_ = "./sync_dir_server";
            std::string default_port_;

            // clients
            client_connection::UserGroup client_manager_;
    };
}