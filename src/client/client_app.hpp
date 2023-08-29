#pragma once

#include <iostream>

// c++
#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <atomic>
#include <cstdlib>
#include <vector>

// multithreading
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>

// argument parsing
#include "../include/common/cxxopts.hpp"

// local
#include "../include/common/inotify_watcher.hpp"
#include "../include/common/lang.hpp"
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
            

        public:
            Client(const std::string& u, const std::string& add, const int& p);
            ~Client();

            // synchronization
            std::string command_buffer;  // shared UI buffer
            std::mutex mutex_;
            std::condition_variable cv_;
            std::string command_buffer_;
            std::vector<std::string> sanitized_commands_;

            void set_sync_dir(std::string new_directory);
            void process_input();
            void main_loop();
            void start();
            void stop();
            void close();
    };
}