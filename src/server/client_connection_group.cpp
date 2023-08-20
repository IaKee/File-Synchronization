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

client_connection::User* UserGroup::get_user(std::string username)
{   
    // iterates though client list
    for(client_connection::User* user : users_)
    {
        if(user->get_username() == username)
        {
            return user;
        }
    }
    return nullptr;
}

void UserGroup::add_user(std::string username)
{
    // checks if user folder exists prior to trying to create it
}

void UserGroup::delete_user(std::string username)
{
    // checks if user folder exists prior to trying to delete it
}

void UserGroup::load_user(std::string username)
{
    // loads user to internal users list
}

int UserGroup::get_active_users_count()
{
    return users_.size();
}