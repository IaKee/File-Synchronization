// standard c++
#include <shared_mutex>
#include <filesystem>

// local
#include "client_app.hpp"
#include "../include/common/utils.hpp"
#include "../include/common/async_cout.hpp"
#include "../include/common/utils_packet.hpp"
#include "../include/common/user_interface.hpp"
#include "../include/common/connection_manager.hpp"
#include "../include/common/inotify_watcher.hpp"

using namespace client_app;
using namespace async_cout;
namespace fs = std::filesystem;

void Client::process_user_interface_commands_()
{
    // process input buffer and calls methods accordingly
    int nargs = ui_sanitized_buffer_.size();

    switch (nargs)
    {
        case 0:
        {
            // no command was entered (enter key pressed)
            break;
        }
        case 1:
        {
            std::string command_name = ui_sanitized_buffer_.front();

            if(command_name == "exit")
            {
                aprint("Stop requested by user...", 1);

                // informs server
                packet exit_packet;
                std::string command_response = "exit";
                strcharray(command_response, exit_packet.command, sizeof(exit_packet.command));
                
                // force sends exit packet to server
                {
                    std::unique_lock<std::mutex> lock(send_mtx_);
                    sender_buffer_.clear();
                    connection_manager_.send_packet(exit_packet);
                }

                // deletes temporary files
                aprint("Deleting temporary download files...", 1);
                delete_temporary_download_files_(this->sync_dir_path_);

                if(inotify_.is_running() == true)
                {
                    inotify_.stop_watching();
                }
                running_app_.store(false);
                running_receiver_.store(false);
                running_sender_.store(false);
                break;
            }
            else if(command_name == "help")
            {
                // TODO: this
                break;
            }
            else
            {
                // user entered malformed command
                this->malformed_command_(command_name);
                break;
            }
        }
        case 2:
        {
            std::string command_name = ui_sanitized_buffer_[0];
            std::string args = ui_sanitized_buffer_[1];

            if(command_name == "download")
            {
                // user requesting an upload to keep in another folder
                request_async_download_(args);
                break;
            }
            else if(command_name == "upload")
            {
                // user uploads file to server - and copies it to local sync dir
                this->upload_command_(args);
                break;
            }
            else if(command_name == "delete")
            {
                // user requests for a file to be deleted on the server
                request_delete_(args);
                break;
            }
            else if(command_name == "list")
            {
                // user requests list command
                this->list_command_(args);
                break;
            }
            else
            {
                aprint("Could not find a valid command by \"" + ui_buffer_ + "\"!", 1);
                break;
            }
        }
        case 3:
        {
            if(ui_sanitized_buffer_[0] == "get" && ui_sanitized_buffer_[1] == "sync" && ui_sanitized_buffer_[2] == "dir")
            {
                aprint("Starting synchronization routine...", 1);
                // temporary files are cleaned on startup

                // requests server to start file synchronization methods
                // TODO: this
                this->start_sync_();
                break;
            }
            else
            {
                aprint("Could not find a valid command by \"" + ui_buffer_ + "\"!", 1);
                break;
            }
        }
        default:
        {
            aprint("Could not find a valid command by \"" + ui_buffer_ + "\"!", 1);
            break;
        }
    }
}

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