// c++
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <vector>
#include <list>
#include <functional>
#include <sstream>
#include <termios.h>

// sync
#include <condition_variable>
#include <mutex>

// locals
#include "lang.hpp"
#include "user_interface.hpp"
#include "utils.hpp"

// namespaces
using namespace user_interface;

UserInterface::UserInterface(
    std::mutex& mtx,
    std::condition_variable& bs,
    std::condition_variable& cs,
    std::string& buff,
    std::list<std::string>& sbuff)
    :   max_fd_(0),
        input_descriptor_(STDIN_FILENO),
        running_(false),
        mutex_(mtx),
        buffer_status_(bs),
        collector_status_(cs),
        command_buffer_(buff),
        sanitized_commands_(sbuff)
    {
        // initialization sequence
    };

void UserInterface::start()
{  // initializes user interface and input loop at another thread
    if(running_)
    {
        // incorrect usage of the start method
        throw std::runtime_error(ERROR_COMMAND_INVALID + "start()");
    }

    running_ = true;
    command_buffer_ = "";  // resets buffer
    buffer_status_.notify_all();
    input_thread_ = std::thread(&UserInterface::input_loop, this);
}

void UserInterface::stop()
{
    if(!running_)
    {  // incorrect usage of the stop method
        
        throw std::runtime_error(ERROR_COMMAND_INVALID + "stop()");
    }


    running_ = false;
    buffer_status_.notify_all();  // notifies other thread to quit waiting
    //collector_status_.notify_all();
    input_thread_.join();

    std::cout << "ui closed" << std::endl;
}

void UserInterface::input_loop()
{  // main interface loop
    while (running_)
    {   
        std::unique_lock<std::mutex> lock(mutex_);
        
        std::cout << "producer: about to wait" << std::endl;
        buffer_status_.wait(
            lock, [this]() 
            {
                return command_buffer_.size() == 0 || !running_;
            });

        // checks if thread should still be running
        if(!running_)
        {
            break;  // exits interface loop
        }

        // retrieves cout file descriptor
        FD_ZERO(&read_fds_);
        FD_SET(input_descriptor_, &read_fds_);
        max_fd_ = input_descriptor_ + 1;  // commonly file descriptors only from 0 to N-1, so the max_fd is N

        // waits to read by using select
        if (select(max_fd_, &read_fds_, nullptr, nullptr, nullptr) == -1)
        {
            std::cerr << PROMPT_PREFIX << ERROR_READING_CRITICAL << std::endl;
            return;
        }

        // checks for reading events on stdin
        if (FD_ISSET(input_descriptor_, &read_fds_))
        {   
            std::cout << "hello from producer!" << std::endl;

            // sanitizes command line, for further processing
            sanitize_user_input();

            // notifies collector that the buffer is ready for processing
            collector_status_.notify_one();

            lock.unlock();  // allows other threads to acess buffers
        }
    }
}

void UserInterface::sanitize_user_input()
{
    // treated as a critical section
    std::string line;
    std::getline(std::cin, line);
    std::istringstream iss(line);
    std::string word;
    while (iss >> word) 
    {
        sanitized_commands_.push_back(word);
    }

    // updates buffer
    command_buffer_ = line;
}

void UserInterface::disable_echo()
{
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void UserInterface::enable_echo()
{
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}