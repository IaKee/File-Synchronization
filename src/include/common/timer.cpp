#include <iostream>
#include <chrono>

#include "timer.hpp"

using namespace TTR;

Timer::Timer() 
    :   start_time_(std::chrono::high_resolution_clock::now()) 
{
    // nothing here
}

Timer::~Timer() 
{
    // prints time elapsed on the scope this instance was created
    if(enable_)
    {
        const auto end_time = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_).count();
        std::cout << " --> ttr: " << format_duration(duration);
    }
}

std::string Timer::format_duration(long long duration) 
{  // formats time duration
    /*
        Private helper function to format the elapsed time into a
        human-readable string. The function determines the magnitude
        (hours, minutes, seconds, milliseconds, or microseconds) and
        returns the formatted string accordingly.
    */
    if (duration >= 3600000000LL) 
    { // More than one hour
        return std::to_string(duration / 3600000000LL) + "h";
    } 
    else if (duration >= 60000000LL) 
    { // More than one minute
        return std::to_string(duration / 60000000LL) + "m";
    } 
    else if (duration >= 1000000LL) 
    { // More than one second
        return std::to_string(duration / 1000000LL) + "s";
    } 
    else if (duration >= 1000LL) { // More than one millisecond
        return std::to_string(duration / 1000LL) + "ms";
    } 
    else 
    {
        return std::to_string(duration) + "us";
    }
}

void Timer::cancel()
{
    this->enable_ = false;
}