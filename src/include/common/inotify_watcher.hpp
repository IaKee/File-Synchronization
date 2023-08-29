#pragma once

#include <functional>
#include <string>
#include <thread>
#include <atomic>
#include <thread>
#include <mutex>
#include <list>

namespace inotify_watcher
{
    struct FileEvent 
    {
        std::string filename;
        uint32_t mask;
    };

    class InotifyWatcher 
    {
        public:
            InotifyWatcher(
                std::string& path_to_watch, 
                std::list<std::string>& inotify_buffer,
                std::mutex& inotify_buffer_mtx);
            InotifyWatcher();
            ~InotifyWatcher();

            void start_watching();
            void stop_watching();
            void monitor_directory();
            void process_event(const FileEvent& event);

        private:
            int watched_path_fd_;
            std::string watched_path_;
            std::thread monitor_thread_;
            std::atomic<bool> is_running_;
            char notify_buffer_[1024];
            std::vector<std::string>& inotify_buffer_;
            std::mutex& inotify_buffer_mtx_;
            
    };
}
