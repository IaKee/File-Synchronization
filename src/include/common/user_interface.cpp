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
#include "user_interface.hpp"
#include "async_cout.hpp"
#include "utils.hpp"

// namespaces
using namespace user_interface;
using namespace async_cout;

UserInterface::UserInterface(
    std::mutex* mutex,
    std::condition_variable* cv,
    std::string* buff,
    std::vector<std::string>* sbuff)
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
        //disable_echo();
    };

UserInterface::~UserInterface()
{
    // re enables echo before closing
    enable_echo();
}

void UserInterface::start()
{  // initializes user interface and input loop at another thread
    try
    {
        if(running_)
        {
            // incorrect usage of the start method
            throw std::runtime_error("\t[USER INTERFACE] Incorrect usage of method start()!");
        }
        aprint("\t[USER INTERFACE] disabling charater echo...");
        disable_echo();

        aprint("\t[USER INTERFACE] initializing...");

        // sets thread control variable to running
        running_ = true;
        //enable_echo();

        // resets shared buffers
        *command_buffer_ = "";  // resets buffer
        sanitized_commands_->clear();
        
        // starts user input thread for command collection
        input_thread_ = std::thread(&UserInterface::input_loop, this);
    }
    catch(const std::exception& e)
    {
        aprint("\t[USER INTERFACE] Error initializing: " + std::string(e.what()));
    }
    catch(...)
    {
        aprint("\t[USER INTERFACE] Unknown error initializing!");
    }
    
}

void UserInterface::stop()
{
    if(!running_ || stop_requested_)
    {  // incorrect usage of the stop method
        throw std::runtime_error("[USER INTERFACE] Incorrect usage of method stop()!");
    }

    running_ = false;
    stop_requested_.store(true);
    aprint("\t[USER INTERFACE] Flags set to stop!");
}

void UserInterface::input_loop()
{  // main interface loop
    try
    {
        while(running_)
        {
            {
                std::unique_lock<std::mutex> lock(*mutex_);
                cv_->wait(lock, [this](){ return command_buffer_->size() == 0 || stop_requested_.load();});
            }
    
            // checks if thread should still be running
            if(stop_requested_.load())
            {
                cv_->notify_one();
                break;
            }

        
            *command_buffer_ = get_buffer();
            //aprint("[UI]" + command_buffer_);
            
            std::istringstream iss(*command_buffer_);
            std::string token;
            while(iss >> token) 
            {
                sanitized_commands_->push_back(token);
            }
           

            //lock.unlock();
            cv_->notify_one();
        }
    }
    catch(const std::exception& e)
    {
        aprint("\t[USER_INTERFACE] Error occured on main_loop(): " + std::string(e.what()));
        throw std::runtime_error(e.what());
    }
    catch(...)
    {
        aprint("\t[USER_INTERFACE] Unknown error occured on main_loop()!");
        throw std::runtime_error("[USER_INTERFACE] Unknown error occured on main_loop()!");
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
        sanitized_commands_->push_back(word);
    }

    // updates buffer
    *command_buffer_ = line;

    std::cout.flush();
}

void UserInterface::remove_commands()
{
    sanitized_commands_->clear();
    *command_buffer_ = "";
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