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
#include "../include/common/async_cout.hpp"

using namespace client_connection;
using namespace async_cout;

UserGroup::UserGroup(
    std::function<void(const packet& p, int sockfd, int timeout)> send_callback,
    std::function<void(packet& p, int sockfd, int timeout)> receive_callback)
    :   send_callback_(send_callback),
        receive_callback_(receive_callback)
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
    aprint("User \"" + username + "\" loaded to vector.", 5);
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
            aprint("User " + username + " removed.", 5);
            return;
        }
        else
        {
            ++it;
        }
    }
}

void UserGroup::global_broadcast(packet& p)
{
    for(client_connection::User* user : users_)
    {
        user->broadcast(p);
    }
}

int UserGroup::get_active_users_count()
{
    return users_.size();
}