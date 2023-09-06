#pragma once

// standard C++
#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <unordered_map>
#include <shared_mutex>

// third-party libraries
#include "../include/common/cxxopts.hpp"

// local includes
#include "../include/common/inotify_watcher.hpp"
#include "../include/common/user_interface.hpp"
#include "../include/common/connection_manager.hpp"
#include "../include/common/utils.hpp"
#include "../include/common/utils_packet.hpp"

using namespace utils_packet;

namespace client_app
{
    class Client
    {
        private:
            // identifiers
            std::string username_;
            std::string machine_name_;
            int session_id_;

            // runtime control
            std::atomic<bool> running_app_;
            std::atomic<bool> running_sender_;
            std::atomic<bool> running_receiver_;

            // internal buffers
            std::string ui_buffer_;
            std::vector<std::string> ui_sanitized_buffer_;
            std::vector<std::string> inotify_buffer_;
            std::vector<packet> sender_buffer_;
            std::vector<packet> receiver_buffer_;
            std::unordered_map<std::string, std::shared_ptr<std::shared_mutex>> file_mtx_;

            // modules
            connection::ClientConnectionManager connection_manager_;
            inotify_watcher::InotifyWatcher inotify_;
            user_interface::UserInterface UI_; 

            // file management
            std::string sync_dir_path_;
            std::string async_dir_path_;

            // mutexes
            std::mutex inotify_buffer_mtx_;
            std::mutex ui_buffer_mtx_;
            std::mutex send_mtx_;
            std::mutex receive_mtx_;

            // condition variables
            std::condition_variable ui_cv_;
            std::condition_variable send_cv_;

            // benchmark
            std::chrono::high_resolution_clock::time_point ping_start_;
            std::chrono::high_resolution_clock::time_point last_ping_;

            // threads
            std::thread sender_th_;
            std::thread receiver_th_;

            // other private methods
            bool set_sync_dir_(std::string new_directory);
            
            // command handlers
            void process_user_interface_commands_();
            void process_inotify_commands_();

            // main server received commands 
            void server_ping_command_();
            void server_list_command_(std::string args, packet buffer);
            void server_download_command_(std::string args, std::string reason = "");
            void server_upload_command_(std::string args, std::string checksum, packet buffer);
            void server_delete_file_command_(std::string args, packet buffer, std::string arg2 = "");
            void server_async_upload_command_(std::string args, std::string checksum, packet buffer);
            void server_exit_command_(std::string reason = "");
            void server_malformed_command_(std::string command);
            
            // main client entered commands
            void request_async_download_(std::string args);
            void request_list_server_(std::string args);
            void request_delete_(std::string args);
            void upload_command_(std::string args, std::string reason = "");
            void pong_command_();
            void list_command_(std::string args);
            void delete_temporary_download_files_(std::string directory);
            void malformed_command_(std::string command);

        public:
            Client(
                std::string username, 
                std::string server_address, 
                int server_port);
            ~Client();

            void main_loop();
            void start();
            void stop();
            void close();

            void start_sender();
            void stop_sender();
            void sender_loop();
            
            void start_receiver();
            void stop_receiver();
            void receiver_loop();
    };
}