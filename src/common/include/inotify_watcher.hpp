#pragma once

#include <functional>
#include <string>
#include <thread>
#include <atomic>
#include <thread>
#include <mutex>
#include <list>
#include <vector>
#include <condition_variable>

#include "asyncio/async_cout.hpp"

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
            InotifyWatcher();
            ~InotifyWatcher();

            void init(
                std::string& path_to_watch, 
                std::vector<std::string>& inotify_buffer,
                std::mutex& inotify_buffer_mtx,
                std::condition_variable& inotify_cv,
                std::vector<std::string>& files_being_modified);

            void start_watching();
            void stop_watching();
            void monitor_directory();
            void process_event(const FileEvent& event);

            bool is_running();

        private:
            int watched_path_fd_;
            int wd;
            
            std::thread monitor_thread_;
            std::atomic<bool> is_running_;
            char notify_buffer_[1024];

            std::string* watched_path_;
            std::vector<std::string>* inotify_buffer_;
            std::mutex* inotify_buffer_mtx_;

            std::condition_variable* inotify_cv_;
            std::vector<std::string>* files_being_modified_;
            
    };
}
