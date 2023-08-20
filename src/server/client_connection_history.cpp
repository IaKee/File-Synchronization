// c++
#include <ctime>
#include <string>
#include <vector>

#include "client_connection.hpp"

using namespace client_connection;

SessionHistory::SessionHistory(
    int socket_fd,
    std::string username,
    std::string address,
    std::string machine,
    std::string sync_dir_path,
    std::vector<std::string> command_history,
    std::time_t start_time,
    std::time_t last_activity,
    int bytes_sent,
    int bytes_recieved,
    int last_ping)
    :   socket_fd(socket_fd),
        username(username),
        address(address),
        machine(machine),
        sync_dir_path(sync_dir_path),
        command_history(command_history),
        start_time(start_time),
        last_activity(last_activity),
        bytes_sent(bytes_sent),
        bytes_recieved(bytes_recieved),
        last_ping(last_ping)
{
    // nothing here
}
