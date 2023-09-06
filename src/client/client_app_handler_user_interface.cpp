// standard c++
#include <shared_mutex>
#include <filesystem>

// local
#include "client_app.hpp"
#include "../include/common/utils.hpp"
#include "../include/common/utils_packet.hpp"
#include "../include/common/user_interface.hpp"
#include "../include/common/connection_manager.hpp"
#include "../include/common/inotify_watcher.hpp"

using namespace client_app;
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
                async_utils::async_print("\t[SYNCWIZARD CLIENT] Stop requested by user...");

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
                async_utils::async_print("\t[SYNCWIZARD CLIENT] Deleting temporary download files...");
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

            if(command_name == "get" && args == "sync_dir")
            {
                if(inotify_.is_running() == false)
                {
                    this->sync_dir_path_ = "./sync_dir";
                    inotify_.init(
                        this->sync_dir_path_, 
                        this->inotify_buffer_, 
                        this->inotify_buffer_mtx_);
                    inotify_.start_watching();
                    async_utils::async_print("\t[SYNCWIZARD CLIENT] Synchronization routine initialized!");
                    break;
                }
                else
                {
                    async_utils::async_print("\t[SYNCWIZARD CLIENT] Synchronization routine is already running!");
                    break;
                }
            }
            else if(command_name == "download")
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
                async_utils::async_print("\t[SYNCWIZARD CLIENT] Could not find a valid command by \"" + ui_buffer_ + "\"!");
                break;
            }
        }
        case 3:
        {
            if(ui_sanitized_buffer_[0] == "get" && ui_sanitized_buffer_[1] == "sync" && ui_sanitized_buffer_[2] == "dir")
            {
                if(!inotify_.is_running())
                {
                    sync_dir_path_ = "./sync_dir";
                    inotify_.init(sync_dir_path_, inotify_buffer_, inotify_buffer_mtx_);
                    inotify_.start_watching();
                    async_utils::async_print("\t[SYNCWIZARD CLIENT] Synchronization routine initialized!");
                    break;
                }
                else
                {
                    async_utils::async_print("\t[SYNCWIZARD CLIENT] Synchronization already initialized!");
                    break;
                }
            }
            else
            {
                async_utils::async_print("\t[SYNCWIZARD CLIENT] Could not find a valid command by \"" + ui_buffer_ + "\"!");
                break;
            }
        }
        default:
        {
            async_utils::async_print("\t[SYNCWIZARD CLIENT] Could not find a valid command by \"" + ui_buffer_ + "\"!");
            break;
        }
    }
}

void Client::process_inotify_commands_()
{
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
            async_utils::async_print("\t[SYNCWIZARD CLIENT] Invalid inotify event!");
            break;
        }
    }

}