// c++
#include <string>
#include <chrono>
#include <exception>
#include <stdexcept>
#include <filesystem>

// locals
#include "client_connection.hpp"
#include "../include/common/json.hpp"
#include "../include/common/utils.hpp"

using namespace client_connection;
namespace fs = std::filesystem;
using json = nlohmann::json;

UserGroup::UserGroup(
    std::function<void(char* buffer, std::size_t buffer_size, int sock_fd, int timeout)> send_callback,
    std::function<void(char* buffer, std::size_t buffer_size, int sock_fd, int timeout)> recieve_callback)
    :   send_callback_(send_callback),
        recieve_callback_(recieve_callback)
{
    //
}

client_connection::User* UserGroup::get_user(std::string username)
{   
    // checks if user is logged in
    for(client_connection::User& user : users_)
    {
        if(user.get_username() == username)
        {
            return &user;
        }
    }
    return nullptr;
}

void UserGroup::delete_user(std::string username)
{
    // checks if user folder exists prior to trying to delete it
}

void UserGroup::load_user(std::string username)
{
    // checks if given username already has a folder on the server, if not creates it
    // adds user to the active users manager (vector)
    std::string user_dir_path = sync_dir_ + "\\" + username;
    
    client_connection::User new_user(username, user_dir_path);

    users_.push_back(new_user);
}

void UserGroup::unload_user(std::string username)
{
    // iterates through sessions
        auto it = users_.begin();
        while(it != users_.end()) 
        {
            if (it->get_username() == username) 
            {   
                // after disconnecting, removes it from the list
                it = users_.erase(it);
                return;
            }
            else
            {
                ++it;
            }
        }
}

int UserGroup::get_active_users_count()
{
    return users_.size();
}