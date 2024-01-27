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

void Client::request_async_download_(std::string args)
{
    // user requesting an upload to keep in another non synchronized folder
    packet adownload_packet;
    std::string file_path = args;
    file_path.erase(
        std::remove_if(
            file_path.begin(), 
            file_path.end(), 
            [](char c) 
            {
                return c == '\'' || c == '\"';
            }), 
        file_path.end());
    
    // mounts download command
    std::string adownload_command = "adownload|" + file_path;
    strcharray(
        adownload_command, 
        adownload_packet.command,
        sizeof(adownload_packet.command));
    
    // requests lock to acess upload buffer
    {
        std::lock_guard<std::mutex> lock(send_mtx_);
        sender_buffer_.push_back(adownload_packet);
    }
    send_cv_.notify_one();
}

void Client::request_list_server_(std::string args)
{
    // user requesting a list of files contained
    packet list_packet;
    
    // mounts list server command
    std::string list_command = "flist|server";
    strcharray(
        list_command, 
        list_packet.command,
        sizeof(list_packet.command));
    
    // requests lock to acess upload buffer
    {
        std::lock_guard<std::mutex> lock(send_mtx_);
        sender_buffer_.push_back(list_packet);
    }
    send_cv_.notify_one();
}

void Client::request_delete_(std::string args)
{
    // user requesting a file deletion from server
    // also deletes the same file locally
    packet delete_packet;
    
    std::string file_path = args;
    file_path.erase(
        std::remove_if(
            file_path.begin(), 
            file_path.end(), 
            [](char c) 
            {
                return c == '\'' || c == '\"';
            }), 
        file_path.end());

    // mounts download command
    std::string delete_command = "delete|" + file_path;
    strcharray(
        delete_command, 
        delete_packet.command,
        sizeof(delete_packet.command));
    
    // requests lock to acess upload buffer
    {
        std::lock_guard<std::mutex> lock(send_mtx_);
        sender_buffer_.push_back(delete_packet);
    }
    send_cv_.notify_one();
}

void Client::upload_command_(std::string args, char type, std::string reason)
{
    // user is sending some file to server - keeping a local copy
    std::string file_path = args;
    file_path.erase(
        std::remove_if(
            file_path.begin(), 
            file_path.end(), 
            [](char c) 
            {
                return c == '\'' || c == '\"';
            }), 
        file_path.end());

    std::string local_file_path;
    if(type == 's')
        local_file_path = sync_dir_path_ + "/" + args;
    else
        local_file_path = args;
    // std::string temp_file_path = local_file_path + ".swizdownload";
    

    if(!is_valid_path(local_file_path))
    {
        aprint("Malformed upload request!", 3);
        std::string output = "Could not acess requested file: \"";
        output += local_file_path + "\"!";
        aprint(output, 3);
        return;
    }
    else
    {
        // given path is valid - creates file lock
        if(file_mtx_.find(file_path) == file_mtx_.end())
        {
            auto new_mutex = std::make_shared<std::shared_mutex>();
            file_mtx_.emplace(args, std::move(new_mutex));
        }
        
        // requests newly created file lock
        {
            std::unique_lock<std::shared_mutex> file_lock(*file_mtx_[file_path]);
    
            std::string checksum = calculate_md5_checksum(local_file_path);
            std::ifstream file(local_file_path, std::ios::binary);

            // also tries to write on temporary file on local sync dir
            // std::ofstream temp_file(temp_file_path, std::ios::app | std::ios::binary);

            if(!file.is_open()) 
            {
                std::string output = "Local machine could not acess given file: ";
                output += "\"" + local_file_path + "\"!";
                aprint(output, 3);
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
                // reads file bufferizing it
                while(!file.eof())
                {
                    // mounts a new buffer packet with default values
                    char* payload_buffer = new char[default_payload];
                    packet upload_buffer;
                    upload_buffer.sequence_number = packet_index;
                    upload_buffer.expected_packets = expected_packets;
                    upload_buffer.payload_size = default_payload;
                    upload_buffer.payload = payload_buffer;
                    
                    // mounts packet command
                    std::string command_response = "upload|" + file_path + "|" + checksum;
                    strcharray(command_response, upload_buffer.command, sizeof(upload_buffer.command));
                    
                    // gathers payload from file stream
                    file.read(upload_buffer.payload, upload_buffer.payload_size);

                    // // writes the same file on local sync dir
                    // temp_file.write(upload_buffer.payload, upload_buffer.payload_size);
                    
                    // adjusts payload size on the last packet to save network bandwith
                    std::size_t bytes_read = static_cast<std::size_t>(file.gcount());
                    if(packet_index == expected_packets - 1) 
                    {
                        // // resizes buffer to actual read buffer size
                        // char* resized_buffer = new char[bytes_read+1];
                        // std::memcpy(resized_buffer, upload_buffer.payload, bytes_read);
                        // delete[] upload_buffer.payload;  // deletes current payload
                        // upload_buffer.payload = resized_buffer;
                        upload_buffer.payload_size = bytes_read;
                    }

                    // adds current file buffer packet to sender buffer
                    {
                        std::unique_lock<std::mutex> lock(send_mtx_);
                        sender_buffer_.push_back(upload_buffer);
                    }
                    send_cv_.notify_one();

                    // increments packet index for the next iteration
                    packet_index++;
                }

                // closes files after being done
                file.close();
                // temp_file.close();

                // removes temp extension from file name
                // rename_replacing(temp_file_path, local_file_path);
                return;
            }
        }
    }
}

void Client::pong_command_()
{
    // server is responding a previous ping request
    auto ping_end = std::chrono::high_resolution_clock::now();
    auto ping_val = std::chrono::duration_cast<std::chrono::microseconds>(
        ping_end - ping_start_);
    double ping_ms = ping_val.count() / 1000.0;
    
    std::string output = "Pinged server with a response time of ";
    output += std::to_string(ping_ms) + "ms.";
    aprint(output, 3);
}

void Client::list_command_(std::string args)
{
    // checks if given list command is valid
    if(args == "server")
    {
        request_list_server_(args);
        return;
    }
    else if(args != "client")
    {
        this->malformed_command_("list " + args);
        return;
    }
    
    aprint(clist_(), 0);
}

std::string Client::clist_()
{
    const std::filesystem::path directory_path{default_sync_dir_path_};
    std::stringstream result;
 
    // directory_iterator can be iterated using a range-for loop
    for (auto const& dir_entry : std::filesystem::directory_iterator{directory_path})
    {
        result << dir_entry.path() << '\n';
    }

    return result.str();
}

int Client::delete_temporary_download_files_(std::string directory)
{
    int deleted_files = 0;

    if(fs::is_directory(directory) && fs::exists(directory)) 
    {
        for(const auto& entry : fs::directory_iterator(directory)) 
        {
            if(fs::is_regular_file(entry) && entry.path().extension() == ".swizdownload") 
            {
                fs::remove(entry);
                deleted_files += 1;
            }
            else if(fs::is_directory(entry)) 
            {
                int deleted_files_subfolder = delete_temporary_download_files_(entry.path());

                if(deleted_files_subfolder > 0)
                {
                    deleted_files += deleted_files_subfolder;
                }
                else
                {
                    std::string eoutput = "Could not delete any temporary files on \"";
                    eoutput += std::string(entry.path()) + "\"";  
                    aprint(eoutput, 3);
                }
            }
        }
        return deleted_files;
    }
    else
    {
        return -1;
    }
}

void Client::malformed_command_(std::string command)
{
    // invalid command request recieved from server
    aprint("Malformed \"" + command + "\" command!", 3);
}