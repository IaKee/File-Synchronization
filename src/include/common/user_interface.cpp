// c++
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <vector>
#include <list>
#include <functional>
#include <exception>
#include <sstream>
#include <termios.h>

// sync
#include <condition_variable>
#include <mutex>

// locals
#include "lang.hpp"
#include "timer.hpp"
#include "user_interface.hpp"
#include "utils.hpp"

// namespaces
using namespace user_interface;

UserInterface::UserInterface(
    std::mutex& mutex,
    std::condition_variable& cv,
    std::string& buff,
    std::list<std::string>& sbuff)
    :   max_fd_(0),
        input_descriptor_(STDIN_FILENO),
        running_(false),
        stop_requested_(false),
        mutex_(mutex),
        cv_(cv),
        command_buffer_(buff),
        sanitized_commands_(sbuff)
    {
        // initialization sequence
        disable_echo();
    };

UserInterface::~UserInterface()
{
    // re enables echo before closing
    enable_echo();
}

void UserInterface::start()
{  // initializes user interface and input loop at another thread
    TTR::Timer timer;

    try
    {
        if(running_)
        {
            // incorrect usage of the start method
            throw std::runtime_error(ERROR_COMMAND_USAGE + "start()");
        }

        // sets thread control variable to running
        running_ = true;
        enable_echo();

        // resets shared buffers
        command_buffer_ = "";  // resets buffer
        sanitized_commands_.clear();
        
        // starts user input thread for command collection
        input_thread_ = std::thread(&UserInterface::input_loop, this);
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error initializing UI: " << e.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Unknown error initializing UI!"  << std::endl;
    }
    
}

void UserInterface::stop()
{
    if(!running_ || stop_requested_)
    {  // incorrect usage of the stop method
        throw std::runtime_error(ERROR_COMMAND_USAGE + "stop()");
    }

    disable_echo();
    running_ = false;
    stop_requested_.store(true);
}

void UserInterface::input_loop()
{  // main interface loop
    try
    {
        while(running_)
        {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this]() {return command_buffer_.size() == 0 || stop_requested_.load();});
            }
    
            // checks if thread should still be running
            if(stop_requested_.load())
            {
                //lock.unlock();
                cv_.notify_one();
                break;
            }

            // retrieves cout file descriptor
            FD_ZERO(&read_fds_);
            FD_SET(input_descriptor_, &read_fds_);
            max_fd_ = input_descriptor_ + 1;  // commonly file descriptors only from 0 to N-1, so the max_fd is N

            // waits to read by using select
            if (select(max_fd_, &read_fds_, nullptr, nullptr, nullptr) == -1)
            {
                std::cerr << PROMPT_PREFIX << ERROR_READING_CRITICAL << std::endl;
                break;
            }

            // checks for reading events on stdin
            if (FD_ISSET(input_descriptor_, &read_fds_))
            {   
                if(!ignoring_)
                {    
                    // sanitizes command line, for further processing
                    add_command();
                }
                else
                {
                    // ignores current user input - no way to actually block it
                    remove_commands();
                }

                // notifies collector that the buffer is ready for processing
            }

            //lock.unlock();
            cv_.notify_one();
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "[ERROR][USER_INTERFACE] Error occured on main_loop(): " << e.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "[ERROR][USER_INTERFACE] Unknown error occured on main_loop()!" << std::endl;
    }
}

void UserInterface::add_command()
{ 
    // already treated as a critical section

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

    std::cout.flush();
}

void UserInterface::remove_commands()
{
    sanitized_commands_.clear();
    command_buffer_ = "";
}

void UserInterface::disable_echo()
{
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    ignoring_ = true;
}

void UserInterface::enable_echo()
{
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    ignoring_ = false;
}