#include <exception>
#include <stdexcept>

#include "client_connection.hpp"
#include "../include/common/async_cout.hpp"

using namespace client_connection;
using namespace async_cout;

void client_connection::aprint(std::string content, int scope, bool endl)
{
    std::string scope_name = "";
    bool valid_scope = false;

    switch (scope)
    {
        case 1:
        {
            scope_name = "SESSION MANAGER";
            valid_scope = true;
            break;
        }
        case 2:
        {
            scope_name = "SESSION COMMANDS";
            valid_scope = true;
            break;
        }
        case 3:
        {
            scope_name = "SESSION NETWORK";
            valid_scope = true;
            break;
        }
        case 4:
        {
            scope_name = "USER MANAGER";
            valid_scope = true;
            break;
        }
        case 5:
        {
            scope_name = "GROUP MANAGER";
            valid_scope = true;
            break;
        }
        default:
        {
            break;
        }
    }

    if(valid_scope)
        scope_name = " - " + scope_name;

    print(content, "client connection" + scope_name, 1, -1, 2, endl);
}

void client_connection::raise(std::string error, int scope)
{
    std::string scope_name = "";
    bool valid_scope = false;

    switch (scope)
    {
        case 1:
        {
            scope_name = "SESSION MANAGER";
            valid_scope = true;
            break;
        }
        case 2:
        {
            scope_name = "SESSION COMMANDS";
            valid_scope = true;
            break;
        }
        case 3:
        {
            scope_name = "SESSION NETWORK";
            valid_scope = true;
            break;
        }
        case 4:
        {
            scope_name = "USER MANAGER";
            valid_scope = true;
            break;
        }
        case 5:
        {
            scope_name = "GROUP MANAGER";
            valid_scope = true;
            break;
        }
        default:
        {
            break;
        }
    }

    if(valid_scope)
        scope_name = " - " + scope_name;

    std::string exception_string = "[CLIENT CONNECTION]" + scope_name + error;
    throw std::runtime_error(exception_string);
}