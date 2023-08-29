// c++
#include <string>
#include <chrono>
#include <exception>
#include <stdexcept>
#include <list>

// locals
#include "client_connection.hpp"
#include "../include/common/json.hpp"
#include "../include/common/utils.hpp"

using namespace client_connection;
using json = nlohmann::json;

UserGroup::UserGroup(
    std::function<void(char* buffer, std::size_t buffer_size, int sock_fd, int timeout)> send_callback,
    std::function<void(char* buffer, std::size_t buffer_size, int sock_fd, int timeout)> recieve_callback)
    :   send_callback_(send_callback),
        recieve_callback_(recieve_callback)
{
    //
}

std::list<std::string> UserGroup::list_users()
{
    std::list<std::string> client_list;
    for(client_connection::User* user : users_)
    {
        std::string output = user->get_username();
        int session_number = user->get_active_session_count();
        output += " (" + std::to_string(session_number) + " sessions)";
        client_list.push_back(output);
    }
    return client_list;
}

client_connection::User* UserGroup::get_user(std::string username)
{   
    // checks if user is logged in
    for(client_connection::User* user : users_)
    {
        if(user->get_username() == username)
        {
            return user;
        }
    }
    return nullptr;
}

void UserGroup::load_user(std::string username)
{  
    std::unique_ptr<client_connection::User> new_user = 
        std::make_unique<client_connection::User>(username, sync_dir_);

    //client_connection::User new_user(username, sync_dir_);
    //users_.push_back(&new_user);
    users_.push_back(new_user.release());
    async_utils::async_print("\t[USER GROUP MANAGER] User \"" + username + "\" loaded to vector.");
}

void UserGroup::unload_user(std::string username)
{
    // removes user from users manager vector
    auto it = users_.begin();
    while(it != users_.end()) 
    {
        if ((*it)->get_username() == username) 
        {   
            // after disconnecting, removes it from the list
            it = users_.erase(it);
            async_utils::async_print("[USER GROUP MANAGER] User " + username + " removed.");
            return;
        }
        else
        {
            ++it;
        }
    }
}

void UserGroup::global_broadcast(char* buffer, std::size_t buffer_size)
{
    for(client_connection::User* user : users_)
    {
        user->broadcast(buffer, buffer_size);
    }
}

int UserGroup::get_active_users_count()
{
    return users_.size();
}