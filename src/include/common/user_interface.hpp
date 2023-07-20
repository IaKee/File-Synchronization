#pragma once

// c++
#include <iostream>
#include <string>
#include <list>

// multithreading
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>

#include <sys/select.h> 
#include <sys/types.h>
#include <unistd.h>


// defining namespace command reader
namespace user_interface
{
    class UserInterface 
    {
        public:
            UserInterface(
                std::mutex& mtx,
                std::condition_variable& bs,
                std::condition_variable& cs,
                std::string& buff,
                std::list<std::string>& sbuff);
            
            void start();
            void stop();
            std::pair<std::string, std::list<std::string>> collect_buffer();

            // synchronization
            std::mutex& mutex_;
            std::condition_variable& buffer_status_;
            std::condition_variable& collector_status_;
            std::string& command_buffer_;
            std::list<std::string>& sanitized_commands_;

        private:
            void sanitize_user_input();
            void input_loop();

            // program flow
            bool running_;
            std::thread input_thread_;
            
            // terminal interface descriptors
            int max_fd_;
            fd_set read_fds_;
            int input_descriptor_;
    };
}
