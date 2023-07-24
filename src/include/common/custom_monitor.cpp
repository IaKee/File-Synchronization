#include <iostream>
#include <mutex>
#include <string>

#include "custom_monitor.hpp"

using namespace custom_monitor;

void CustomMonitor::m_lock(std::mutex& mtx, const std::string& method_name)
{
    if(debug_messages_)
        std::cout << "[DEBUG]" + method_name + " is waiting for the mutex..." << std::endl;

    std::unique_lock<std::mutex> lock(monitor_mutex_);
    owner_ = method_name;
    mtx.lock();

    if(debug_messages_)
        std::cout << "[DEBUG]" + owner_ + " got the mutex." << std::endl;
}

void CustomMonitor::m_unlock(std::mutex& mtx) 
{
    if(debug_messages_)
        std::cout << "[DEBUG]" + owner_ + " is freeing the mutex..." << std::endl;

    mtx.unlock();
    std::unique_lock<std::mutex> lock(monitor_mutex_);
    owner_ = "";

    if(debug_messages_)
        std::cout << "[DEBUG] the mutex is now free." << std::endl;

}

std::string CustomMonitor::get_current_owner() 
{
    std::unique_lock<std::mutex> lock(monitor_mutex_);
    return owner_;
}

void CustomMonitor::toggle_debug()
{
    debug_messages_ != debug_messages_;
}
