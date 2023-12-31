# pragma once

#include <iostream>
#include <string>

namespace async_cout
{
    void print(
        std::string content, 
        std::string module_name,
        int indent, 
        int fg, 
        int bg, 
        bool endl = true);
    void altprint(std::string content, bool endl = true);

    std::string agets();
    
    void add_to_buffer(char c);
    void backspace_buffer();
    std::string get_buffer();

    // terminal input capturing
    void start_capture();
    void stop_capture();
    void capture_loop();
}