#pragma once

#include <vector>
#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <list>
#include <sys/select.h> 
#include <poll.h>


// defining namespace command reader
namespace command_reader
{
    class CommandReader 
    {
        public:
            CommandReader(
                const std::vector<std::pair<std::string, std::function<void(const std::string&)>>>& commands);

            void start();
            void stop();

        private:
            void handle_user_input();

            std::vector<std::pair<std::string, std::function<void(const std::string&)>>> commands_;
            std::atomic<bool> should_run_;

            int max_fd_;
            fd_set read_fds_;
            int input_descriptor_; 
            std::string buffer;  // current command entered by 
            std::list<std::thread> child_threads_;  // children - spawned thread tasks
    };
}  // namespace command_reader
