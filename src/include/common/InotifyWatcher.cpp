#include "InotifyWatcher.hpp"
#include <iostream>
#include <sys/inotify.h>
#include <unistd.h>

const int EVENT_SIZE = sizeof(struct inotify_event);
const int BUF_LEN = 1024;

InotifyWatcher::InotifyWatcher(const std::string& path, EventCallback callback)
    : path_(path), callback_(callback), isRunning_(false) {}

InotifyWatcher::~InotifyWatcher() {
    stopWatching();
}

void InotifyWatcher::startWatching() {
    int fd = inotify_init();
    if (fd < 0) {
        std::cerr << "Error initializing inotify" << std::endl;
        return;
    }

    int wd = inotify_add_watch(fd, path_.c_str(), IN_CREATE | IN_MODIFY | IN_DELETE);
    if (wd < 0) {
        std::cerr << "Error adding watch" << std::endl;
        return;
    }

    isRunning_ = true;
    monitorThread_ = std::thread(&InotifyWatcher::monitorDirectory, this, fd);

    monitorThread_.detach();
}

void InotifyWatcher::stopWatching() {
    isRunning_ = false;
}

void InotifyWatcher::monitorDirectory(int fd) {
    char buffer[EVENT_SIZE + BUF_LEN];
    while (isRunning_) {
        int length = read(fd, buffer, EVENT_SIZE + BUF_LEN);
        if (length < 0) {
            std::cerr << "Error reading from inotify" << std::endl;
            break;
        }

        int i = 0;
        while (i < length) {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];

            if (event->mask & IN_CREATE || event->mask & IN_MODIFY || event->mask & IN_DELETE) {
                if (event->mask & IN_ISDIR) {
                    // Handle directory changes if needed
                } else {
                    FileEvent fileEvent;
                    fileEvent.filename = event->name;
                    fileEvent.mask = event->mask;

                    callback_(fileEvent); // Notifica a classe chamadora sobre o evento
                }
            }

            i += EVENT_SIZE + event->len;
        }
    }

    inotify_rm_watch(fd, IN_ALL_EVENTS);
    close(fd);
}
