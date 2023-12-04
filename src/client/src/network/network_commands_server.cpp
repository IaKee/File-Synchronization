// standard c++
#include <filesystem>
#include <fstream>

// c
#include <dirent.h>
#include <sys/stat.h>

#include "../application.hpp"
#include "../../../common/include/asyncio/async_cout.hpp"

using namespace client_application;
using namespace async_cout;
namespace fs = std::filesystem;

void Client::server_ping_command_()
{
    // server is measuring connection ping with this client
    // mounts a response packet and adds to send queue
    packet ping_packet;
    std::string command_string = "pong";
    strcharray(command_string, ping_packet.command, sizeof(ping_packet.command));
    
    // adds to sender buffer
    {
        std::unique_lock<std::mutex> lock(send_mtx_);
        sender_buffer_.push_back(ping_packet);
    }
    send_cv_.notify_one();
}

void Client::server_list_command_(std::string args, packet buffer)
{
    // server is responding to a list command
    if(args == "server")
    {
        std::vector<std::string> file_list = split_buffer(buffer.payload);
        
        if(file_list.size() > 0)
        {
            std::string output = "Currently these files are being hosted for your user:";

            for(std::string file : file_list)
            {
                output += file;
            }

            // prints file list
            aprint(output, 4);
            return;
        }
        else
        {
            std::string output = "No files were found on the server for your user!";
            aprint(output, 4);
            return;
        }
    }
    else if(args == "fail")
    {
        aprint("List server command failed!", 4);
        
        std::string reason = "Captured error: ";
        reason += std::string(buffer.payload, buffer.payload_size);
        aprint(reason, 4);
        return;
    }
    else
    {
        this->server_malformed_command_("list");
        return;
    }
}

void Client::server_download_command_(std::string args, std::string reason)
{
    // server is requesting some file from user
    std::string file_path = args;
    std::string local_file_path = sync_dir_path_ + args;

    if(!is_valid_path(local_file_path))
    {
        packet fail_packet;
        std::string command_response = "sdownload|" + file_path + "|fail";
        strcharray(command_response, fail_packet.command, sizeof(fail_packet.command));
        std::string args_response = "Given file was not found on user local machine.";
        fail_packet.payload = new char[args_response.size()+1];
        strcharray(args_response, fail_packet.payload, args_response.size()+1);
        fail_packet.payload_size = args_response.size();

        // adds fail packet to sender buffer
        {
            std::unique_lock<std::mutex> lock(send_mtx_);
            sender_buffer_.push_back(fail_packet);
        }
        send_cv_.notify_one();

        aprint("Malformed upload request recieved from server!", 4);
        aprint("Could not acess server requested file: \"" + file_path + "\"!", 4);
        return;
    }
    else
    {
        // given path is valid - requests file lock
        if(file_mtx_.find(args) != file_mtx_.end())
        {
            std::unique_lock<std::shared_mutex> file_lock(*file_mtx_[args]);
            
            std::string checksum = calculate_md5_checksum(local_file_path);
            std::ifstream file(local_file_path, std::ios::binary);

            if(!file.is_open()) 
            {
                packet fail_packet;
                std::string command_response = "sdownload|" + file_path + "|fail";
                strcharray(command_response, fail_packet.command, sizeof(fail_packet.command));
                std::string args_response = "Local machine could not acess given file!";
                strcharray(args_response, fail_packet.payload, args_response.size());
                fail_packet.payload_size = args_response.size();

                // adds fail packet to sender buffer
                {
                    std::unique_lock<std::mutex> lock(send_mtx_);
                    sender_buffer_.push_back(fail_packet);
                }
                send_cv_.notify_one();
                return;
            }
            else
            {        
                // packet defaults
                int default_payload = 8192;  // 8kb
                int packet_index = 0;
                file.seekg(0, std::ios::end);
                std::size_t file_size = file.tellg();
                file.seekg(0, std::ios::beg);
                size_t expected_packets = std::max((file_size + default_payload - 1) / default_payload, (size_t) 1);
                
                // reads file bufferizingit
                while(!file.eof())
                {

                    // mounts a new buffer packet with default values
                    packet sdownload_buffer;
                    sdownload_buffer.sequence_number = packet_index;
                    sdownload_buffer.expected_packets = expected_packets;
                    sdownload_buffer.payload_size = default_payload;
                    
                    // mounts packet command
                    std::string command_response = "sdownload|" + file_path + "|" + checksum;
                    strcharray(command_response, sdownload_buffer.command, sizeof(sdownload_buffer.command));
                    
                    // gathers payload from file stream
                    file.read(sdownload_buffer.payload, sdownload_buffer.payload_size);
                    
                    // adjusts payload size on the last packet to save network bandwith
                    std::size_t bytes_read = static_cast<std::size_t>(file.gcount());
                    if(packet_index == expected_packets - 1) 
                    {
                        // resizes buffer to actual read buffer size
                        char* resized_buffer = new char[bytes_read];
                        std::memcpy(resized_buffer, sdownload_buffer.payload, bytes_read);
                        delete[] sdownload_buffer.payload;  // deletes current payload
                        sdownload_buffer.payload = resized_buffer;
                    }

                    // adds current file buffer packet to sender buffer
                    {
                        std::unique_lock<std::mutex> lock(send_mtx_);
                        sender_buffer_.push_back(sdownload_buffer);
                    }
                    send_cv_.notify_one();

                    // increments packet index for the next iteration
                    packet_index++;
                }

                // closes file after being done
                file.close();
                return;
            }
        }
        else
        {
            // if there is no mutex for this file, an error has occured!
            raise("No file mutex was found for \"" + args + "\"!", 4);
            return;
        }
    }
}

void Client::server_upload_command_(std::string args, std::string checksum, packet buffer)
{
    // server is sending some file to async download folder
    std::stringstream ss;
    ss << sync_dir_path_<< '/' << args;
    std::string local_file_path = ss.str();
    std::string temp_file_path = local_file_path + ".swizdownload";
    
    if(checksum == "fail")
    {
        // user requested file download failed
        std::string output = "User requested";
        output += " file async download for \"" + args + "\" failed!";
        aprint(output, 4);
        std::string reason = "Captured error:";
        reason += std::string(buffer.payload, buffer.payload_size);
        aprint(reason, 4);
        return;
    }

    // tries to write on temporary file
    std::ofstream temp_file(temp_file_path, std::ios::app | std::ios::binary);
    if(!temp_file) 
    {
        // given file does not exist locally - informs server
        packet fail_packet;
        std::string command_response = "aupload|" + args + "|fail";
        strcharray(command_response, fail_packet.command, sizeof(fail_packet.command));
        std::string args_response = "Given file could not be created or accessed";
        args_response += "on user local machine.";
        strcharray(args_response, fail_packet.payload, args_response.size());
        fail_packet.payload_size = args_response.size();

        // adds to sender buffer
        {
            std::unique_lock<std::mutex> lock(send_mtx_);
            sender_buffer_.push_back(fail_packet);
        }
        send_cv_.notify_one();

        aprint("Could not write on file sent by server!", 4);
        aprint("Could not acess file: \"" + args + "\"!", 4);
        return;
    }
    else 
    {
        // updates temporary file with payload
        temp_file.write(buffer.payload, buffer.payload_size);
        temp_file.close();

        // checks if the packet is last to overwrite original file
        if(buffer.sequence_number == buffer.expected_packets - 1)
        {
            if(file_mtx_.find(args) == file_mtx_.end())
            {
                auto new_mutex = std::make_shared<std::shared_mutex>();
                file_mtx_.emplace(args, std::move(new_mutex));
            }

            // requests file mutex to change original file
            std::unique_lock<std::shared_mutex> file_lock(*file_mtx_[args]);
                
            std::string current_checksum = calculate_md5_checksum(temp_file_path);

            if(current_checksum != checksum)
            {
                std::string output = "File md5 checksum for ";
                output += "\"" + args + "\" is different than the informed amount!";
                aprint(output, 4);
            }
            else
            {
                if(log_checksum) 
                {
                    std::string output = "File md5 checksum for ";
                    output += "\"" + args + "\" is exactly the informed amount!";
                    aprint(output, 4);
                }
            }

            // deletes temporary file replacing the original file
            if(is_valid_path(local_file_path) && calculate_md5_checksum(local_file_path) == current_checksum)
            {   
                delete_file(temp_file_path);
            }
            else
            {
                rename_replacing(temp_file_path, local_file_path);
            }
            return;
        }

        return;
    }
}

void Client::server_delete_file_command_(std::string args, packet buffer, std::string arg2)
{
    std::string local_file_path = sync_dir_path_ + "/" + args;
    
    if(arg2 == "fail")
    {
        // user requested file download failed
        std::string output = "User requested delete command for \"" + args + "\" failed!";
        aprint(output, 4);
        std::string reason = "Captured error: ";
        reason += std::string(buffer.payload, buffer.payload_size);
        aprint(reason, 4);
        return;
    }

    if(!is_valid_path(local_file_path))
    {
        // given file does not exist locally - informs server
        packet fail_packet;
        std::string command_response = "delete|" + args + "|fail";
        strcharray(command_response, fail_packet.command, sizeof(fail_packet.command));
        std::string args_response = "Given file was not found on user local machine.";
        fail_packet.payload = new char[args_response.size()+1];
        strcharray(args_response, fail_packet.payload, args_response.size()+1);
        fail_packet.payload_size = args_response.size();

        // adds to sender buffer
        {
            std::unique_lock<std::mutex> lock(send_mtx_);
            sender_buffer_.push_back(fail_packet);
        }
        send_cv_.notify_one();
        
        aprint("Invalid file path. Delete command ignored.", 4);
        return;
    }
    else if(args == "fail")
    {
        // previously user-sent delete command request failed
        aprint("Server could not process malformed delete command!", 4);
        
        // if a rejection reason was given, print it
        if(buffer.payload_size > 0)
        {
            std::string output = "Server rejected delete command with: ";
            output += std::string(buffer.payload, buffer.payload_size);
            aprint(output, 4);
        }
        return;
    }
    else
    {
        // valid path, tries to delete file
        try
        {
            // requests file mutex
            if(file_mtx_.find(args) != file_mtx_.end())
            {
                std::unique_lock<std::shared_mutex> file_lock(*file_mtx_[args]);
                delete_file(local_file_path);

                // removes file mutex from internal list
                auto it = file_mtx_.find(args);
                if(it != file_mtx_.end())
                {
                    file_mtx_.erase(it);
                    return;
                }
                else
                {
                    std::string output = "Could not find a file mutex for \"";
                    output += args + "\"!";
                    raise(output, 3);
                    return;
                }
                return;
            }
            else
            {
                // if there is no mutex for this file, an error has occured!
                raise("No file mutex was found for \"" + args + "\"!", 4);
            }
        }
        catch(const std::exception& e)
        {
            std::string output = "Exception raised while deleting file: ";
            output += std::string(e.what());
            aprint(output, 4);

            // sends fail packet to server
            packet fail_packet;
            std::string command_response = "delete|" + args + "|fail";
            strcharray(command_response, fail_packet.command, sizeof(fail_packet.command));
            std::string args_response = std::string(e.what());
            strcharray(args_response, fail_packet.payload, args_response.size());
            fail_packet.payload_size = args_response.size();

            // adds to sender buffer
            {
                std::unique_lock<std::mutex> lock(send_mtx_);
                sender_buffer_.push_back(fail_packet);
            }
            send_cv_.notify_one();
            return;
        }
    }
}

void Client::server_async_upload_command_(std::string args, std::string checksum, packet buffer)
{
    // server is sending some file to async download folder
    std::string local_file_path = async_dir_path_ + args;
    std::string temp_file_path = local_file_path + ".swizdownload";
    
    if(checksum == "fail")
    {
        // user requested file download failed
        std::string output = "User requested";
        output += " file async download for \"" + args + "\" failed!";
        aprint(output, 4);
        std::string reason = "Captured error:";
        reason += std::string(buffer.payload, buffer.payload_size);
        aprint(reason, 4);
        return;
    }

    // tries to write on temporary file
    std::ofstream temp_file(temp_file_path, std::ios::app | std::ios::binary);
    if(!temp_file) 
    {
        // given file does not exist locally - informs server
        packet fail_packet;
        std::string command_response = "aupload|" + args + "|fail";
        strcharray(command_response, fail_packet.command, sizeof(fail_packet.command));
        std::string args_response = "Given file could not be created or accessed";
        args_response += "on user local machine.";
        strcharray(args_response, fail_packet.payload, args_response.size());
        fail_packet.payload_size = args_response.size();

        // adds to sender buffer
        {
            std::unique_lock<std::mutex> lock(send_mtx_);
            sender_buffer_.push_back(fail_packet);
        }
        send_cv_.notify_one();

        aprint("Could not write on file sent by server!", 4);
        aprint("Could not acess file: \"" + args + "\"!", 4);
        return;
    }
    else 
    {
        // updates temporary file with payload
        temp_file.write(buffer.payload, buffer.payload_size);
        temp_file.close();

        // checks if the packet is last to overwrite original file
        if(buffer.sequence_number == buffer.expected_packets - 1)
        {
            if(file_mtx_.find(args) == file_mtx_.end())
            {
                auto new_mutex = std::make_shared<std::shared_mutex>();
                file_mtx_.emplace(args, std::move(new_mutex));
            }

            // requests file mutex to change original file
            std::unique_lock<std::shared_mutex> file_lock(*file_mtx_[args]);
                
            std::string current_checksum = calculate_md5_checksum(temp_file_path);

            if(current_checksum != checksum)
            {
                std::string output = "File md5 checksum for ";
                output += "\"" + args + "\" is different than the informed amount!";
                aprint(output, 4);
            }
            else
            {
                std::string output = "File md5 checksum for ";
                output += "\"" + args + "\" is exactly the informed amount!";
                aprint(output, 4);
            }

            // deletes temporary file replacing the original file
            rename_replacing(temp_file_path, local_file_path);
            return;
        }

        return;
    }
}

void Client::server_exit_command_(std::string reason)
{
    // received logout confirmation from server
    aprint("Logout requested by server!", 4);

    if(reason.size() > 0)
    {
        aprint("Logout reason: " + reason, 4);
    }

    if(inotify_.is_running() == true)
    {
        inotify_.stop_watching();
    }
    running_app_.store(false);
    running_receiver_.store(false);
    running_sender_.store(false);
    return;
}

void Client::server_malformed_command_(std::string command)
{
    // invalid command request recieved from server
    std::string output = "Received malformed \"" + command + "\" command from server!";
    aprint(output, 4);
}