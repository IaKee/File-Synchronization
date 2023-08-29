#pragma once

#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <atomic>
#include <cstdlib>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>

// third-party libraries
#include "../include/common/cxxopts.hpp"

// local includes
#include "../include/common/inotify_watcher.hpp"
#include "../include/common/user_interface.hpp"
#include "../include/common/connection.hpp"
#include "../include/common/utils.hpp"


// namespace
using cxxopts::Options;
using cxxopts::ParseResult;

namespace client_app
{
    class Client
    {
        private:
            // runtime control
            bool running_;
            std::atomic<bool> stop_requested_;

            // connection
            std::string username_;
            std::string server_address_;
            connection::Connection connection_;
            int server_port_;

            // file management
            std::string sync_dir_;

            // input
            user_interface::UserInterface UI_; 

            inotify_watcher::InotifyWatcher inotify_;
            std::vector<std::string> inotify_buffer_;
            std::mutex inotify_buffer_mtx_;

        public:
            Client(const std::string& u, const std::string& add, const int& p);
            ~Client();

            // synchronization
            std::string command_buffer;  // shared UI buffer
            std::mutex mutex_;
            std::condition_variable cv_;
            std::vector<std::string> sanitized_commands_;

            std::mutex send_mtx_;
            std::condition_variable send_cv_;
            std::atomic<bool> running_sender_;
            std::thread sender_th_;
            std::list<std::pair<char*, std::size_t>> sender_buffer_;

            std::atomic<bool> running_receiver_;
            std::thread receiver_th_;
            std::mutex recieve_mtx_;
            std::size_t expected_buffer_size = 1024;
            std::list<std::pair<char*, std::size_t>> receiver_buffer_;
            const char EOF_MARKER = 0xFF;

            std::string file_listing_string_;

            // other
            std::atomic<int> timeout_sec = 5;
            std::chrono::high_resolution_clock::time_point ping_start_;
            std::chrono::high_resolution_clock::time_point last_ping_;

            void set_sync_dir(std::string new_directory);
            void process_input();
            void main_loop();
            void start();
            void stop();
            void close();

            void send(char* buffer, std::size_t size, int timeout = -1);
            void recieve(char* buffer, std::size_t size, int timeout = -1);

            void start_sender();
            void stop_sender();
            void sender_loop();
            
            void start_receiver();
            void stop_receiver();
            void receiver_loop();
    };
}