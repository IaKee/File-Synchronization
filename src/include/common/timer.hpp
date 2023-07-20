# pragma once

#include <iostream>
#include <chrono>
#include <string>

namespace TTR
{
    class Timer 
    {
        /*
        Class used to time simple tasks, aiming to have a lightweight implementation
        and easy usage. Upon being destroyed, it will display the time elapsed on the
        scope it was declared.

        Example usage:
            Timer timer;
            // When leaving this scope, the time elapsed will be computed
            // and displayed on stdout.
        */

        public:
            // on init
            Timer();

            // on destroy
            ~Timer();

            // cancels timing - at least does not print it
            void cancel();

            // control variable, if set to false, won't print timers
        private:
            std::chrono::time_point<std::chrono::high_resolution_clock> start_time_;
            std::string format_duration(long long duration);
            bool enable_ = true;
    };
}