#include "../application.hpp"
#include "../../common/include/async_cout.hpp"
#include "../../common/include/inotify_watcher.hpp"

using namespace client_application;

void Client::process_inotify_commands_()
{
    // TODO: this
    int inotify_nargs = inotify_buffer_.size();
    switch (inotify_nargs)
    {
        case 3:
        {
            std::string type = inotify_buffer_[1];
            std::string file_path = inotify_buffer_[2];
            if(type == "create" || type == "modify")
            {
                // upload file to server
                break;
            }
            else if(type == "delete")
            {
                // request file deletion on server
                break;
            }
        }
        default:
        {
            aprint("Invalid inotify event!", 1);
            break;
        }
    }

}