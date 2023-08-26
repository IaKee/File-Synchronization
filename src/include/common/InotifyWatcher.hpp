#ifndef INOTIFYWATCHER_HPP
#define INOTIFYWATCHER_HPP

#include <functional>
#include <string>
#include <thread>

class InotifyWatcher {
public:
    struct FileEvent {
        std::string filename;
        uint32_t mask;
    };

    using EventCallback = std::function<void(const FileEvent&)>;

    InotifyWatcher(const std::string& path, EventCallback callback);
    ~InotifyWatcher();

    void startWatching();
    void stopWatching();

private:
    std::string path_;
    EventCallback callback_;
    std::thread monitorThread_;
    bool isRunning_;

    void monitorDirectory(int fd);
};

#endif // INOTIFYWATCHER_HPP
