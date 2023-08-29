// c++
#include <iostream>
#include <list>

// c
#include <sys/inotify.h>
#include <unistd.h>

// local
#include "inotify_watcher.hpp"
#include "utils.hpp"

const int EVENT_SIZE = sizeof(struct inotify_event);
const int BUF_LEN = 1024;

using namespace inotify_watcher;

InotifyWatcher::InotifyWatcher(
    std::string& path_to_watch, 
    std::list<std::string>& inotify_buffer,
    std::mutex& inotify_buffer_mtx)
    :   watched_path_(path_to_watch), 
        inotify_buffer_(inotify_buffer), 
        inotify_buffer_mtx_(inotify_buffer_mtx),
        is_running_(false) 
{

};

InotifyWatcher::InotifyWatcher()
{

};

InotifyWatcher::~InotifyWatcher() 
{
    stop_watching();
}

void InotifyWatcher::start_watching() 
{
    int watched_path_fd_ = inotify_init();
    
    if(watched_path_fd_ < 0) 
    {
        throw std::runtime_error("[INOTIFY WATCHER] Error initializing.");
        is_running_.store(false);
        return;
    }

    int wd = inotify_add_watch(watched_path_fd_, watched_path_.c_str(), IN_CREATE | IN_MODIFY | IN_DELETE);
    
    if(wd < 0) 
    {
        throw std::runtime_error("[INOTIFY WATCHER] Error adding watch.");
        is_running_.store(false);
        return;
    }

    is_running_.store(true);
    monitor_thread_ = std::thread(
        [this]()
        {
            monitor_directory();
        });
}

void InotifyWatcher::stop_watching() 
{
    is_running_.store(false);
    if(monitor_thread_.joinable())
    {
        monitor_thread_.join();
    }

    close(watched_path_fd_);
}

void InotifyWatcher::monitor_directory() 
{
    while (is_running_.load()) 
    {
        int length = read(watched_path_fd_, notify_buffer_, sizeof(notify_buffer_));
        if (length < 0) 
        {
            throw std::runtime_error("[INOTIFY WATCHER] Error reading from inotify.");
            is_running_.store(false);
            return;
        }

        int i = 0;
        while (i < length) 
        {
            struct inotify_event *event = reinterpret_cast<struct inotify_event*>(&notify_buffer_[i]);

            if(event->mask & (IN_CREATE | IN_MODIFY | IN_DELETE)) 
            {
                if(!(event->mask & IN_ISDIR)) 
                {
                    FileEvent file_event;
                    file_event.filename = event->name;
                    file_event.mask = event->mask;

                    process_event(file_event);
                }
            }

            i += sizeof(struct inotify_event) + event->len;
        }
    }

    inotify_rm_watch(watched_path_fd_, IN_ALL_EVENTS);
    close(watched_path_fd_);
}

void InotifyWatcher::process_event(const FileEvent& event) 
{
    if (event.mask & IN_CREATE) 
    {
        std::unique_lock<std::mutex> lock(inotify_buffer_mtx_);
        inotify_buffer_.push_back("inotify|create|" + event.filename);
    }

    if (event.mask & IN_MODIFY) 
    {
        std::unique_lock<std::mutex> lock(inotify_buffer_mtx_);
        inotify_buffer_.push_back("inotify|modify|" + event.filename);
    }

    if (event.mask & IN_DELETE) 
    {
        std::unique_lock<std::mutex> lock(inotify_buffer_mtx_);
        inotify_buffer_.push_back("inotify|delete|" + event.filename);
    }
}