#include <iostream>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <vector>
#include <list>

#include "command_reader.hpp"
#include "utils.hpp"
#include "constants.hpp"

using namespace command_reader;

CommandReader::CommandReader(const std::vector<std::pair<std::string, std::function<void(const std::string&)>>>& commands)
    :   commands_(commands),
        should_run_(false),
        max_fd_(0),  // largest file descriptor
        input_descriptor_(STDIN_FILENO) {}

void CommandReader::start()
{
    // main loop
    should_run_ = true;

    // commonly file descriptors only from 0 to N-1, so the max_fd is N
    max_fd_ = input_descriptor_ + 1;

    // main reading loop
    while (should_run_)
    {
        // configure file descriptiors for reading
        FD_ZERO(&read_fds_);
        FD_SET(input_descriptor_, &read_fds_);

        // waits to read by using select
        if (select(max_fd_, &read_fds_, nullptr, nullptr, nullptr) == -1)
        {
            std::cerr << "Reading error while using select! :(" << std::endl;
            return;
        }

        // checks for reading events on stdin
        if (FD_ISSET(input_descriptor_, &read_fds_))
        {
            handle_user_input();
        }
    }
}

void CommandReader::stop()
{
    should_run_ = false;
}

void CommandReader::handle_user_input()
{
    // command line sanitizing
    std::getline(std::cin, buffer);
    std::vector<std::string> words;
    std::istringstream iss(buffer);
    std::string word;

    while (iss >> word) 
    {
        words.push_back(word);
    }

    bool has_method = false;
    bool has_argument = false;
    switch (words.size())
    {
        case 0:
            insert_prefix();
            return;
            break;
        case 1:
            has_method = true;
            break;
        case 2:
            has_method = true;
            has_argument = true;
            break;
        default:
            insert_prefix();
            std::cout << INVALID_COMMAND << std::endl;
            insert_prefix();
            break;
    }

    // terminates command reader program by typing exit
    if (buffer == "exit")
    {
        insert_prefix();
        std::cout << EXIT_MESSAGE << std::endl;
        should_run_ = false;
        return;
    }

    std::string command_name = words[0];
    std::string command_argument = words[1];
    if(has_method)
    {
        // process user input
        bool command_found = false;
        for (const auto& command : commands_)
        {
            
            // iterates all available commands
            if (command_name == command.first)
            {
                command_found = true;

                // calls the respective method
                // TODO: check if multithreading is needed when evoking new method
                if(has_argument)
                    command.second(command_argument);  
                else
                    command.second(std::string());  

                // a valid command was found, stops iterating
                break;
            }
        }

        if (!command_found)
        {
            insert_prefix();
            std::cout << INVALID_COMMAND << std::endl;
            insert_prefix();
        }
    }
    
}