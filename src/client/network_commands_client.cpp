// standard c++
#include <filesystem>
#include <fstream>

// c
#include <dirent.h>
#include <sys/stat.h>

#include "client_app.hpp"

using namespace client_app;
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
    // user requesting an upload to keep in another non synchronized folder
    packet list_packet;
    
    // mounts download command
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

    // deletes file locally
    std::string local_file_path = sync_dir_path_ + file_path;
    if(is_valid_path(local_file_path) == false)
    {
        std::string output = "\t[SYNCWIZARD CLIENT] Could not acess given file!";
        async_utils::async_print(output);
        return;
    }
    
    // requests file mutex to delete entry
    if(file_mtx_.find(file_path) == file_mtx_.end())
    {
        std::unique_lock<std::shared_mutex> file_lock(*file_mtx_[file_path]);
        
        // deletes file
        delete_file(local_file_path);

        // removes file mutex from internal list
        auto it = file_mtx_.find(file_path);
        if(it != file_mtx_.end())
        {
            file_mtx_.erase(it);
            return;
        }
        else
        {
            std::string output = "\t[SYNCWIZARD CLIENT] Could not find a file mutex for \"";
            output += file_path + "\"!";
            throw std::runtime_error(output);
            return;
        }
    }
    else
    {
        std::string output = "\t[SYNCWIZARD CLIENT] Could not find a file mutex for \"";
        output += args + "\"!";
        throw std::runtime_error(output);
        return;
    }
}

void Client::upload_command_(std::string args, std::string reason)
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

    std::string local_file_path = sync_dir_path_ + args;
    std::string temp_file_path = local_file_path + ".swizdownload";
    

    if(!is_valid_path(local_file_path))
    {
        async_utils::async_print("\t[SYNCWIZARD CLIENT] Malformed upload request!");
        std::string output = "\t[SYNCWIZARD CLIENT] Could not acess requested file: \"";
        output += local_file_path + "\"!";
        async_utils::async_print(output);
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
            std::ofstream temp_file(temp_file_path, std::ios::app | std::ios::binary);

            if(!file.is_open()) 
            {
                std::string output = "\t[SYNCWIZARD CLIENT] Local machine could not acess given file: ";
                output += "\"" + local_file_path + "\"!";
                async_utils::async_print(output);
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
                size_t expected_packets = (file_size + default_payload - 1) / default_payload;
                
                // reads file bufferizingit
                while(!file.eof())
                {
                    // mounts a new buffer packet with default values
                    packet upload_buffer;
                    upload_buffer.sequence_number = packet_index;
                    upload_buffer.expected_packets = expected_packets;
                    upload_buffer.payload_size = default_payload;
                    
                    // mounts packet command
                    std::string command_response = "upload|" + file_path + "|" + checksum;
                    strcharray(command_response, upload_buffer.command, sizeof(upload_buffer.command));
                    
                    // gathers payload from file stream
                    file.read(upload_buffer.payload, upload_buffer.payload_size);

                    // writes the same file on local sync dir
                    temp_file.write(upload_buffer.payload, upload_buffer.payload_size);
                    
                    // adjusts payload size on the last packet to save network bandwith
                    std::size_t bytes_read = static_cast<std::size_t>(file.gcount());
                    if(packet_index == expected_packets - 1) 
                    {
                        // resizes buffer to actual read buffer size
                        char* resized_buffer = new char[bytes_read];
                        std::memcpy(resized_buffer, upload_buffer.payload, bytes_read);
                        delete[] upload_buffer.payload;  // deletes current payload
                        upload_buffer.payload = resized_buffer;
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
                temp_file.close();

                // removes temp extension from file name
                rename_replacing(temp_file_path, local_file_path);
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
    
    std::string output = "\t[SYNCWIZARD CLIENT] Pinged server with a response time of ";
    output += std::to_string(ping_ms) + "ms.";
    async_utils::async_print(output);
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

    DIR* dir = opendir(sync_dir_path_.c_str());
    if(dir == nullptr) 
    {
        throw std::runtime_error("[SYNCWIZARD CLIENT] Could not access local sync_dir folder!");
    }

    std::string output = "\t[SYNCWIZARD CLIENT] Currently these files are being hosted on sync_dir:";
    std::function<void(const std::string&)> list_files_recursively = [&](const std::string& current_path) 
    {
        DIR* dir = opendir(current_path.c_str());
        if(dir == nullptr) 
        {
            std::string reason = "\t[SYNCWIZARD CLIENT] Could not access local sync_dir folder!";
            throw std::runtime_error(reason);
        }

        
        dirent* entry;
        while((entry = readdir(dir)) != nullptr) 
        {
            if(entry->d_type == DT_REG) 
            { 
                std::string file_path = current_path + "/" + entry->d_name;
                file_path += "/";
                file_path += entry->d_name;

                struct stat file_info;
                if(lstat(file_path.c_str(), &file_info) == 0) 
                {
                    char modification_time_buffer[100];
                    char access_time_buffer[100];
                    char change_creation_time_buffer[100];

                    output += "\n\t\t\tFile name: " + std::string(entry->d_name);
                    output += "\n\t\t\tFile path: " + file_path;

                    std::strftime(
                        modification_time_buffer, 
                        sizeof(modification_time_buffer), 
                        "%c", 
                        std::localtime(&file_info.st_mtime));
                    output += "\n\t\t\tModification time: " + std::string(modification_time_buffer);

                    std::strftime(
                        access_time_buffer, 
                        sizeof(access_time_buffer), 
                        "%c", 
                        std::localtime(&file_info.st_atime));
                    output += "\n\t\t\tAccess time: " + std::string(access_time_buffer);

                    std::strftime(
                        change_creation_time_buffer, 
                        sizeof(change_creation_time_buffer), 
                        "%c", 
                        std::localtime(&file_info.st_ctime));
                    output += "\n\t\t\tChange/creation time: " + std::string(change_creation_time_buffer);

                }
                else if(entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) 
                {
                    list_files_recursively(current_path + "/" + entry->d_name);
                }
            }
        }
        closedir(dir);
    };

    list_files_recursively(sync_dir_path_);
    
    async_utils::async_print(output);
    return;
}

void Client::delete_temporary_download_files_(std::string directory)
{
    if(fs::is_directory(directory) && fs::exists(directory)) 
    {
        for(const auto& entry : fs::directory_iterator(directory)) 
        {
            if(fs::is_regular_file(entry) && entry.path().extension() == ".swizdownload") 
            {
                fs::remove(entry);
            }
            else if(fs::is_directory(entry)) 
            {
                delete_temporary_download_files_(entry.path()); // Recursivamente, para subpastas.
            }
        }
    }
}

void Client::malformed_command_(std::string command)
{
    // invalid command request recieved from server
    std::string output = "\t[SYNCWIZARD CLIENT] Malformed \"";
    output += command + "\" command!";
    async_utils::async_print(output);
}