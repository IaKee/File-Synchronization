#include "connection_manager.hpp"
#include "../asyncio/async_cout.hpp"

using namespace connection;
using namespace async_cout;

void connection::aprint(std::string content, int scope, bool endl)
{
    std::string scope_name = "";
    bool valid_scope = false;

    switch (scope)
    {
        case 1:
        {
            scope_name = "CLIENT";
            valid_scope = true;
            break;
        }
        case 2:
        {
            scope_name = "SERVER";
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

    print(content, "connection manager" + scope_name, 1, 0, 3, endl);
}

void connection::raise(std::string error, int scope)
{
    std::string scope_name = "";
    bool valid_scope = false;

    switch (scope)
    {
        case 1:
        {
            scope_name = "CLIENT";
            valid_scope = true;
            break;
        }
        case 2:
        {
            scope_name = "SERVER";
            valid_scope = true;
            break;
        }
        default:
        {
            break;
        }
    }

    if(valid_scope)
        scope_name = "[" + scope_name + "]";

    std::string exception_string = "[CONNECTION MANAGER]" + scope_name + error;
    throw std::runtime_error(exception_string);
}