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
#include "../common/include/inotify_watcher.hpp"
#include "../common/include/user_interface.hpp"
#include "../common/include/network/connection_manager.hpp"
#include "../common/include/utils.hpp"
#include "../common/include/network/packet.hpp"

using namespace utils_packet;

namespace client_application
{
    // modified output strings to show code scopes
    void aprint(std::string content, int scope = -1, bool endl = true);
    void raise(std::string error, int scope = -1);

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
            std::atomic<bool> running_sync_;

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
            std::string sync_dir_path_ = "";
            std::string async_dir_path_ = "";
            const std::string default_sync_dir_path_ = "./sync dir";
            const std::string default_async_dir_path_ = "./downloads";

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
            void start_sync_(std::string new_path = "");

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
            int delete_temporary_download_files_(std::string directory);
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