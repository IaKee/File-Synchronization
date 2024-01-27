// standard c++
#include <shared_mutex>
#include <filesystem>

// local
#include "../application.hpp"
#include "../../../common/include/utils.hpp"
#include "../../../common/include/asyncio/async_cout.hpp"
#include "../../../common/include/network/packet.hpp"
#include "../../../common/include/asyncio/user_interface.hpp"
#include "../../../common/include/network/connection_manager.hpp"
#include "../../../common/include/inotify_watcher.hpp"

using namespace client_application;
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
                std::string help_text = "SyncWizard client is a file synchronization program.\n"
                       "It allows you to synchronize your files on a remote server for access and sharing.\n"
                       "All non-help command-line arguments are required to run. Below are the details about each one:\n"
                       "\n"
                       "Usage:\n"
                       "SyncWizard Client [OPTION...]\n"
                       "\n"
                       "-u, --username arg   Use this option to specify the username to which\n"
                       "                    the files will be associated. This is important for\n"
                       "                    identifying the synchronized files belonging to\n"
                       "                    each user. It must be between 1 and 12 characters\n"
                       "                    long, without spaces or symbols.\n"
                       "-s, --server_ip arg  Use this option to specify the IP address of the\n"
                       "                    SyncWizard server you want to use. This will allow\n"
                       "                    the program to connect to the correct server for\n"
                       "                    file synchronization.\n"
                       "-p, --port arg       Use this option to specify the port number of\n"
                       "                    the SyncWizard server you want to use. The program\n"
                       "                    will use this port to establish a connection\n"
                       "                    with the server.\n"
                       "-h, --help           This option displays the description of the\n"
                       "                    available program arguments. If you need help or\n"
                       "                    have questions about how to use SyncWizard, you\n"
                       "                    can use this option to get more information.\n"
                       "-e, --example        Example usage: ./SWizClient --username john\n"
                       "                    --server_ip 192.168.0.100 --port 8080";

                aprint(help_text, 1);
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
                // requests server to start file synchronization methods
                // clears any temporary files on the given folder (or on the default one)
                // gathers a local file list and sends to server, which then
                // compares to its local files and requests/sends unsynchronized ones
                aprint("Starting synchronization routine...", 1);

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