// c++
#include <string>
#include <chrono>
#include <exception>
#include <stdexcept>

// locals
#include "client_connection.hpp"
#include "lang.hpp"


using namespace client_connection;

User* UserGroup::get_user(std::string username)
{   
    // iterates though client list
    for(User u : users_)
    {
        if(u.get_username() == username)
        {
            return &u;
        }
    }
    return nullptr;
}

void UserGroup::login(std::string username)
{
    // checks if user is already logged in
    if(UserGroup::get_user(username) != nullptr)
    {
        std::cout << ERROR_USER_LOGGED << std::endl;
        throw std::runtime_error(ERROR_USER_LOGGED);
    }

    // tries to read previously saved user data
    if(filesys_.path_exists(".\\" + username))
    {
        // reads user data
    }
    else
    {
        // user is new, or could not find user folder
    }

}

int UserGroup::active_users_count()
{
    return users_.size();
}