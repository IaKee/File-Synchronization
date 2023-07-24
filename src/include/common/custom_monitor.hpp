#include <mutex>
#include <string>

namespace custom_monitor
{

    class CustomMonitor 
    {
        public:
            CustomMonitor();
            ~CustomMonitor();

            void m_lock(std::mutex& mtx, const std::string& new_owner);
            void m_unlock(std::mutex& mtx);
            std::string get_current_owner();
            void toggle_debug();

        private:
            std::mutex monitor_mutex_;
            std::string owner_;
            bool debug_messages_;
    };
}
