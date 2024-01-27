#include "../application.hpp"
#include "../../../common/include/asyncio/async_cout.hpp"
#include "../../../common/include/inotify_watcher.hpp"

using namespace client_application;

void Client::process_inotify_commands_()
{
    // TODO: talvez mudar inotify buffer para deque, ja que estamos sempre
    // tirando o primeiro elemento
    char* command = new char[inotify_buffer_.front().size()+1];
    strcharray(inotify_buffer_.front(), command, inotify_buffer_.front().size()+1);
    std::vector<std::string> sanitized_buffer = split_buffer(command);
    int inotify_nargs = sanitized_buffer.size();
    inotify_buffer_.erase(inotify_buffer_.begin());
    
    switch (inotify_nargs)
    {
        case 3:
        {
            std::string type = sanitized_buffer[1];
            std::string file_path = sanitized_buffer[2];
            if(type == "create" || type == "modify")
            {
                // upload file to server
                upload_command_(file_path, 's');
                break;
            }
            else if(type == "delete")
            {
                // request file deletion on server
                request_delete_(file_path);
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