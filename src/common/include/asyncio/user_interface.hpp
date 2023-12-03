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
    void aprint(std::string content, bool endl = true);
    void raise(std::string content);
    
    class UserInterface 
    {
        public:
            UserInterface(
                std::mutex* mutex,
                std::condition_variable* cv,
                std::string* buff,
                std::vector<std::string>* sbuff);
            ~UserInterface();
            
            void start();
            void stop();
            void add_command();
            void remove_commands();
            void async_print(std::string content);

            // synchronization
            std::mutex* mutex_;
            std::condition_variable* cv_;
            std::string* command_buffer_;
            std::vector<std::string>* sanitized_commands_;
            std::thread input_thread_;


        private:
            void input_loop();
            void enable_echo();
            void disable_echo();

            // program flow
            bool running_;
            std::atomic<bool> stop_requested_;
            bool ignoring_;
            
            // terminal interface descriptors
            int max_fd_;
            fd_set read_fds_;
            int input_descriptor_;

    };
}
