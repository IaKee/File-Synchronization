# pragma once

// c++
#include <iostream>
#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>

// connection
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

// locals
#include "./network/client_connection.hpp"
#include "./network/server_connection.hpp"
#include "../../common/include/network/connection_manager.hpp"
#include "../../common/include/asyncio/user_interface.hpp"
#include "../../common/include/utils.hpp"

using json = nlohmann::json;

namespace server
{
    void aprint(std::string content, bool endl = true);
    void raise(std::string error);
    
    class Server
    {
        public:
            // synchronization
            std::mutex ui_mutex;
            std::condition_variable ui_cv;
            std::string ui_buffer;
            std::vector<std::string> ui_sanitized_buffer;

            // init & destroy
            Server();
            ~Server();

            // methods
            void start();
            void stop();
            void close();
            void handle_new_session(int new_socket, std::string username, std::string machine);
            void handle_new_server(int new_socket, std::string username, std::string machine);
            void process_input();
            void main_loop();
 
            // modules
            connection::ServerConnectionManager internet_manager;
        private:
            // runtime control
            std::atomic<bool> running_;
            std::atomic<bool> stop_requested_;

            // other attributes
            std::string client_default_path_;
            std::string sync_dir_ = "./sync_dir_server";
            std::string default_port_;

            // threads
            std::thread accept_th_;

            // modules
            user_interface::UserInterface S_UI_;
            client_connection::UserGroup client_manager_;
            server_connection::ServerGroup server_manager_;
            
    };
}