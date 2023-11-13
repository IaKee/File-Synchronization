// standard c++
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <algorithm>

// standard c
#include <ctype.h>
#include <unistd.h>
#include <sys/select.h>

// local includes
#include "async_cout.hpp"

using namespace async_cout;

// main buffers
std::queue<std::string> input_buffer;
std::string terminal_buffer;

std::mutex cout_mtx;
std::mutex input_mtx;

// condition variables
std::condition_variable cout_cv;
std::condition_variable input_cv;

// other
std::atomic<bool> capturing = false;
std::thread capture_th;

void async_cout::print(std::string content, std::string module_name, int indent, int fg, int bg, bool endl)
{
    // string modifier for color coding module names
    bool has_modifier = false;
    std::string string_starter = "";
    std::string string_ender = "\033[0m";
    std::string new_string = "";
    
    if(bg >= 0)
    {
        std::string BACKGROUND_COLORS[] = 
        {
            "\033[48;5;231m",  // white
            "\033[48;5;196m",  // red
            "\033[48;5;40m",   // green
            "\033[48;5;51m",   // blue
            "\033[48;5;226m",  // yellow
            "\033[48;5;21m",   // magenta
            "\033[48;5;27m",   // cyan
            "\033[48;5;124m"   // orange
        };

        try
        {
            std::string chosen_color = BACKGROUND_COLORS[bg];
            string_starter += chosen_color;
            has_modifier = true;
        }
        catch(const std::exception& e)
        {
            string_starter += "[invalid bg] ";
        }
        
    }

    if(fg >= 0)
    {
        std::string FOREGROUND_COLORS[] = 
        {
            "\033[38;5;0m",   // black
            "\033[38;5;15m",  // white
            "\033[38;5;255m", // whitest
            "\033[38;5;202m", // bright red
            "\033[38;5;82m",  // bright green
            "\033[38;5;33m",  // bright blue
            "\033[38;5;201m", // bright yellow
            "\033[38;5;129m"  // dark orange
        };

        try
        {
            std::string chosen_color = FOREGROUND_COLORS[fg];
            string_starter += chosen_color;
            has_modifier = true;
        }
        catch(const std::exception& e)
        {
            string_starter += "[invalid fg] ";
        }
    }

    if(has_modifier == false)
    {
        string_ender = "";
    }

    // sends string to output
    {
        std::lock_guard<std::mutex> alock(cout_mtx);
        printf("\r\033[K"); // deletes current line
        
        if(endl)
        {
            content += '\n';
        }
        content += "\t#> " + terminal_buffer;
        
        if(module_name.size() > 0)
        {
            std::transform(
                module_name.begin(), 
                module_name.end(), 
                module_name.begin(), 
                ::toupper);
            module_name = "[" + module_name + "]";
            
            std::cout 
                << std::string(indent, ' ') 
                << string_starter 
                << module_name 
                << string_ender
                << " " 
                << content 
                << std::flush;
        }
        else
        {
            std::cout 
                << std::string(indent, ' ') 
                << string_starter 
                << string_ender 
                << " "
                << content 
                << std::flush;
        }
    }    

}

void async_cout::altprint(std::string content, bool endl)
{
    std::lock_guard<std::mutex> alock(cout_mtx);

    printf("\r\033[K"); // deletes current line
    if(endl)
    {
        content += '\n';
    }
    content += "\t#> " + terminal_buffer;
    
    std::cout << content << std::flush;
}

std::string async_cout::agets()
{
    std::string input;
    
    // gets the input line
    {
        std::lock_guard<std::mutex> alock(cout_mtx);
        std::getline(std::cin, input);
    }

    // echoes to cout
    altprint(input);

    return input;
}

void async_cout::add_to_buffer(char c)
{
    terminal_buffer += c;
}

void async_cout::backspace_buffer()
{
    terminal_buffer.pop_back();
}

std::string async_cout::get_buffer()
{
    {
        std::unique_lock<std::mutex> alock(input_mtx);
        input_cv.wait(alock, [](){return input_buffer.size() != 0;});
    }

    if(!input_buffer.empty())
    {
        std::string buf = input_buffer.front();
        input_buffer.pop();

        return buf;
    }
    return "";
}

void async_cout::start_capture()
{
    capturing.store(true);

    capture_th = std::thread(capture_loop);
}

void async_cout::stop_capture()
{
    capturing.store(false);

    if(capture_th.joinable())
    {
        capture_th.join();
    }
    else
    {
        throw std::runtime_error("[ASYNC UTILS] Capture thread is not joinable!");
    }
}

void async_cout::capture_loop()
{
    struct timeval input_timeout;
    input_timeout.tv_sec = 1;
    input_timeout.tv_usec = 0;

    char c;
    while(capturing.load() == true)
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        int result = select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, &input_timeout);
        
        if(result > 0 && FD_ISSET(STDIN_FILENO, &read_fds)) 
        {
            c = std::cin.get();
            {        
                switch(c)
                {
                    case '\n':
                    {
                        std::lock_guard<std::mutex> alock(input_mtx);
                        input_buffer.push(terminal_buffer);
                        std::string strtmp = std::string(terminal_buffer);
                        terminal_buffer.clear();
                        altprint("\t#> " + strtmp);
                        input_cv.notify_one();
                        break;
                    }
                    case ' ':
                    {
                        add_to_buffer(' ');
                        altprint("", false);
                        break;
                    }
                    case 127:
                    {
                        if(!terminal_buffer.empty())
                        {
                            backspace_buffer();
                            altprint("", false);
                        }
                        break;
                    }
                    default:
                    {
                        if(isalnum(c))
                        {
                            add_to_buffer(c);
                            altprint("", false);
                        }
                        break;
                    }
                }
            }
        }
    }
}