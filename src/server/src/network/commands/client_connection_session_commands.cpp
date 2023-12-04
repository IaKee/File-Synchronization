// standard c++
#include <iostream>
#include <filesystem>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <fstream>

// c
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

// locals
#include "../client_connection.hpp"
#include "../../../../common/include/utils.hpp"
#include "../../../../common/include/asyncio/async_cout.hpp"
#include "../../../../common/include/network/packet.hpp"

using namespace async_cout;
using namespace client_connection;
namespace fs = std::filesystem;

void ClientSession::malformed_command_(std::string command)
{
    if(command.size() == 0)
    {
        std::string output = get_identifier() + " Received empty string as command.";
        aprint(output, 2);
    }
    else
    {
        std::string output = get_identifier() + " Received malformed command: " + command;
        aprint(output, 2);
    }
}

void ClientSession::client_requested_logout_()
{
    // received logout request from user
    std::string output = get_identifier() + " Requested logoff.";
    aprint(output, 2);

    running_receiver_.store(false);
    running_sender_.store(false);
    user->remove_session(socket_fd_, "");
}

void ClientSession::client_requested_ping_()
{
    // server is measuring connection ping with client
    // mounts a response packet and adds to send queue
    packet pong_packet;
    std::string command_string = "pong";
    strcharray(command_string, pong_packet.command, sizeof(pong_packet.command));
    
    // adds to sender buffer
    {
        std::unique_lock<std::mutex> lock(send_mtx_);
        sender_buffer_.push_back(pong_packet);
    }
    send_cv_.notify_one();
}

void ClientSession::client_responded_ping_()
{
    // received ping response from user
    auto ping_end = std::chrono::high_resolution_clock::now();
    auto ping_val = std::chrono::duration_cast<
        std::chrono::microseconds>(ping_end - ping_start_);
    double ping_ms = ping_val.count() / 1000.0; // Em milissegundos
    
    std::string output = get_identifier() + " Pinged with a response time of ";
    output += std::to_string(ping_ms) + "ms.";
    aprint(output, 2);
}

void ClientSession::client_requested_delete_(std::string args, packet buffer, std::string arg2)
{
    // client requested to delete certain file
    // propagates to other sessions
    std::string local_file_path = directory_path_ + "/" + this->get_username() + "/" + args;
    std::string file_name = args;

    if(!is_valid_path(local_file_path))
    {
        std::string output = get_identifier() + " Delete command for file \"" + file_name;
        output += "\" failed! Could not acess given path!";
        aprint(output, 2);

        // mounts fail packet
        packet fail_packet;
        std::string command_string = "delete|" + file_name + "|fail";
        strcharray(command_string, fail_packet.command, sizeof(fail_packet.command));
        std::string reason = "Could not find or acess given file path!";
        fail_packet.payload = new char[reason.size()+1];
        strcharray(reason, fail_packet.payload, reason.size()+1);
        fail_packet.payload_size = reason.size()+1;
        
        // requests send mutex
        {
            std::unique_lock<std::mutex> lock(send_mtx_);
            sender_buffer_.push_back(fail_packet);
        }
        send_cv_.notify_one();
        return;
    }
    else
    {
        // valid path, propagates, then deletes file

        // propagates user delete command
        broadcast_user_callback_(socket_fd_, buffer);

        try
        {
            // requests file mutex to delete entry
            aprint(file_name, 0);
            if(file_mtx_.find(file_name) == file_mtx_.end())
            {
                auto new_mutex = std::make_shared<std::shared_mutex>();
                file_mtx_.emplace(file_name, std::move(new_mutex));
                aprint("created mutex", 0);
            }

            aprint("vai locar?", 0);
            std::unique_lock<std::shared_mutex> file_lock(*file_mtx_[file_name]);
            aprint("locou", 0);

            aprint(directory_path_, 0);
            
            // deletes file
            aprint(local_file_path, 0);
            delete_file(local_file_path);

            // after deleting - removes file mutex from internal list
            auto it = file_mtx_.find(file_name);
            if(it != file_mtx_.end())
            {
                file_mtx_.erase(it);
                return;
            }
            else
            {
                std::string output = get_identifier() + " Could not find a file mutex for \"";
                output += file_name + "\"!";
                raise(output, 2);
            }
        }
        catch(const std::exception& e)
        {
            aprint("morri aq", 0);
            std::string output = get_identifier() + + " Exception raised while deleting file \"";
            output += file_name + "\": " + std::string(e.what());
            raise(output, 2);
        }
    }
}

void ClientSession::client_requested_slist_()
{
    // client requested a list of every file hosted on server
    std::string output = slist_();

    // mounts packet to send
    packet slist_packet;
    std::string command = "slist";
    strcharray(command, slist_packet.command, sizeof(slist_packet.command));
    int payload_size = output.size();
    strcharray(output, slist_packet.payload, payload_size);
    slist_packet.payload_size = payload_size;

    // adds to sender buffer
    {
        std::unique_lock<std::mutex> lock(send_mtx_);
        sender_buffer_.push_back(slist_packet);
    }

    output = get_identifier() + " Sent slist to session.";
    aprint(output, 2);
}

void ClientSession::client_requested_flist_()
{
    // client requested formatted list of every file

    const std::filesystem::path directory_path{directory_path_ + "/" + user->get_username() + "/"};
    std::stringstream result;
 
    // directory_iterator can be iterated using a range-for loop
    for (auto const& dir_entry : std::filesystem::directory_iterator{directory_path})
    {
        result << dir_entry.path() << '\n';
        aprint(dir_entry.path(), 0);
    }

    std::string output = result.str();
    aprint(output, 0);
        
    // mounts packet to send
    packet flist_packet;
    std::string command = "flist|server";
    strcharray(command, flist_packet.command, sizeof(flist_packet.command));
    int payload_size = output.size()+1;
    flist_packet.payload = new char[payload_size];
    strcharray(output, flist_packet.payload, payload_size);
    flist_packet.payload_size = payload_size;

    // adds to sender buffer
    {
        std::unique_lock<std::mutex> lock(send_mtx_);
        sender_buffer_.push_back(flist_packet);
    }

    send_cv_.notify_one();
    output = get_identifier() + " Sent file list to session.";
    aprint(output, 2);
}

void ClientSession::client_requested_adownload_(std::string args)
{
    // user is requesting a file download to
    // keep in a non synchronized folder
    // send as "aupload"
    std::string file_path = args;
    std::string local_file_path = directory_path_ + "/" + this->get_username() + "/" + args;

    if(is_valid_path(local_file_path) == false)
    {
        // invalid path, sends fail packet
        packet fail_packet;
        std::string command = "aupload|" + args + "|fail";
        strcharray(command, fail_packet.command, sizeof(fail_packet.command));
        std::string reason = "Could not find or acess given file!";
        fail_packet.payload = new char[reason.size()+1];
        fail_packet.payload_size = reason.size();
        strcharray(reason, fail_packet.payload, reason.size() + 1);

        // adds current file buffer packet to sender buffer
        {
            std::unique_lock<std::mutex> lock(send_mtx_);
            sender_buffer_.push_back(fail_packet);
        }
        send_cv_.notify_one();

        std::string output = get_identifier() + " Async download failed! Could not acess given file: ";
        output += "\"" + args + "\"!";
        aprint(output, 2);
    }
    else
    {
        // given path is valid - requests file lock
        if(file_mtx_.find(args) == file_mtx_.end())
        {
            {
                auto new_mutex = std::make_shared<std::shared_mutex>();
                file_mtx_.emplace(args, std::move(new_mutex));
            }

            {
                aprint("1", 0);
                std::unique_lock<std::shared_mutex> file_lock(*file_mtx_[args]);
                aprint("2", 0);
                
                std::string checksum = calculate_md5_checksum(local_file_path);
                std::ifstream sfile(local_file_path, std::ios::binary);

                if(!sfile.is_open()) 
                {
                    aprint("2", 0);
                    std::string output = get_identifier() + " Server could not bufferize file to send: " + args;
                    aprint(output, 2);
                    
                    // mounts packet
                    packet fail_packet;
                    std::string command = "aupload|" + args + "|fail";
                    strcharray(command, fail_packet.command, sizeof(fail_packet.command));
                    std::string reason = "Server could not bufferize file to send!";
                    fail_packet.payload_size = reason.size();
                    strcharray(reason, fail_packet.payload, reason.size());

                    // adds current file buffer packet to sender buffer
                    {
                        std::unique_lock<std::mutex> lock(send_mtx_);
                        sender_buffer_.push_back(fail_packet);
                    }
                    send_cv_.notify_one();
                    return;
                }
                else
                {   
                    aprint("3", 0);
                    // valid file     

                    // set packet defaults
                    int default_payload = 8192;  // 8kb
                    int packet_index = 0;
                    sfile.seekg(0, std::ios::end);
                    std::size_t file_size = sfile.tellg();
                    sfile.seekg(0, std::ios::beg);
                    size_t expected_packets = (file_size + default_payload - 1) / default_payload;
                    
                    // reads file bufferizing it
                    while(!sfile.eof())
                    {
                        // mounts a new buffer packet with default values
                        char* payload_buffer = new char[default_payload];
                        packet supload_packet;
                        supload_packet.sequence_number = packet_index;
                        supload_packet.expected_packets = expected_packets;
                        supload_packet.payload_size = default_payload;
                        supload_packet.payload = payload_buffer;
                        
                        // mounts packet command
                        std::string command_response = "aupload|" + args + "|" + checksum;
                        strcharray(command_response, supload_packet.command, sizeof(supload_packet.command));
                        
                        // gathers payload from file stream
                        sfile.read(supload_packet.payload, supload_packet.payload_size);
                        
                        // adjusts payload size on the last packet to save network bandwith
                        std::size_t bytes_read = static_cast<std::size_t>(sfile.gcount());
                        if(packet_index == expected_packets - 1) 
                        {
                            // aprint("4", 0);
                            // // resizes buffer to actual read buffer size
                            // char* resized_buffer = new char[bytes_read];
                            // aprint(std::to_string(bytes_read), 0);
                            // std::memcpy(resized_buffer, supload_packet.payload, bytes_read);
                            // delete[] supload_packet.payload;  // deletes current payload
                            supload_packet.payload_size = bytes_read;
                        }

                        // adds current file buffer packet to sender buffer
                        {
                            std::unique_lock<std::mutex> lock(send_mtx_);
                            sender_buffer_.push_back(supload_packet);
                        }
                        send_cv_.notify_one();

                        // increments packet index for the next iteration
                        packet_index++;
                    }

                    // closes files after done
                    sfile.close();
                }
            }
        }
        else
        {
            // if there is no mutex for this file, an error has occured!
            std::string output = get_identifier() + " No file mutex was found for file \"" + args + "\"!";
            aprint(output, 2);
            return;
        }
    }

    aprint(args, 0);
    aprint(local_file_path, 0);
}

void ClientSession::client_sent_clist_(packet buffer, std::string args)
{
    // client sent a list of file currently on their local machine
    // request updates to make server and session up to date
    // creates upload/download requests for files not synchronized
    // ignores temporary files

    if(args == "fail")
    {
        // client could not retrieve listed files
        std::string output = get_identifier() + " Client could not retrieve synchronized files";
        aprint(output, 2);
    }

    // session files
    std::vector<std::string> session_files = split_buffer(buffer.payload);

    // current server files
    std::string current_server_files_string = slist_();
    std::istringstream iss(current_server_files_string);
    std::string token;
    std::vector<std::string> current_server_files;
    while(std::getline(iss, token, '|')) 
    {
        current_server_files.push_back(token);
    }

    // file difference between session and server
    std::vector<std::string> files_not_in_session;
    std::vector<std::string> files_not_in_current_server;
    std::vector<std::string> server_temporary_files;
    std::vector<std::string> session_temporary_files;

    // moves temporary download files from current files buffer to the propper one
    for(auto it = current_server_files.begin(); it != current_server_files.end();) 
    {
        if((*it).find(".swizdownload") != std::string::npos)
        {
            server_temporary_files.push_back((*it));
            it = current_server_files.erase(it);
        } 
        else 
        {
            ++it;
        }
    }

    // does the same thing for session files
    for(auto it = session_files.begin(); it != session_files.end();) 
    {
        if((*it).find(".swizdownload") != std::string::npos)
        {
            session_temporary_files.push_back((*it));
            it = session_files.erase(it);
        } 
        else 
        {
            ++it;
        }
    }

    // checks for unsynchronized session files
    for(const std::string& file : current_server_files) 
    {
        if(std::find(
            session_files.begin(), 
            session_files.end(), 
            file) == session_files.end()) 
        {
            // session does not have current file
            std::string temporary_file_name = file_without_extension(file) + ".swizdownload";

            // checks for temporary files
            if(std::find(
                session_temporary_files.begin(), 
                session_temporary_files.end(), 
                temporary_file_name) == session_temporary_files.end())
            {
                // session does not have current file at all
                files_not_in_session.push_back(file);
            }
        }
    }

    // checks for unsynchronized server files
    for(const std::string& file : session_files) 
    {
        if(std::find(
            current_server_files.begin(), 
            current_server_files.end(), 
            file) == current_server_files.end()) 
        {
            // server does not have current file
            std::string temporary_file_name = file_without_extension(file) + ".swizdownload";

            //checks for temporary files
            if(std::find(
                server_temporary_files.begin(), 
                server_temporary_files.end(), 
                temporary_file_name) == server_temporary_files.end())
            {
                files_not_in_current_server.push_back(file);
            }
        }
    }

    int file_diff = files_not_in_session.size() + files_not_in_current_server.size();
    std::string output = get_identifier();
    output += "Currently there's a difference of " + std::to_string(file_diff) + "files.";
    output += " Server will request accordingly commands to update session.";
    aprint(output, 2);

    int delta_packets = 0;
    // updates session with missing files
    for(const std::string& file : files_not_in_session) 
    {
        std::string local_file_path = directory_path_ + file;

        // requests file lock
        if(file_mtx_.find(file) != file_mtx_.end())
        {
            std::unique_lock<std::shared_mutex> file_lock(*file_mtx_[file]);
            
            std::string checksum = calculate_md5_checksum(local_file_path);
            std::ifstream sfile(local_file_path, std::ios::binary);

            if(!sfile.is_open()) 
            {
                std::string output = get_identifier() + " Server could not bufferize file to send: " + file;
                aprint(output, 2);
                
                // jumps to the next file...
                continue;
            }
            else
            {   
                // valid file     
                // set packet defaults
                int default_payload = 8192;  // 8kb
                int packet_index = 0;
                sfile.seekg(0, std::ios::end);
                std::size_t file_size = sfile.tellg();
                sfile.seekg(0, std::ios::beg);
                size_t expected_packets = (file_size + default_payload - 1) / default_payload;
                
                // reads file bufferizing it
                while(!sfile.eof())
                {
                    // mounts a new buffer packet with default values
                    packet supload_packet;
                    supload_packet.sequence_number = packet_index;
                    supload_packet.expected_packets = expected_packets;
                    supload_packet.payload_size = default_payload;
                    
                    // mounts packet command
                    std::string command_response = "sdownload|" + file + "|" + checksum;
                    strcharray(command_response, supload_packet.command, sizeof(supload_packet.command));
                    
                    // gathers payload from file stream
                    sfile.read(supload_packet.payload, supload_packet.payload_size);
                    
                    // adjusts payload size on the last packet to save network bandwith
                    std::size_t bytes_read = static_cast<std::size_t>(sfile.gcount());
                    if(packet_index == expected_packets - 1) 
                    {
                        // resizes buffer to actual read buffer size
                        char* resized_buffer = new char[bytes_read];
                        std::memcpy(resized_buffer, supload_packet.payload, bytes_read);
                        delete[] supload_packet.payload;  // deletes current payload
                        supload_packet.payload = resized_buffer;
                    }

                    // adds current file buffer packet to sender buffer
                    {
                        std::unique_lock<std::mutex> lock(send_mtx_);
                        delta_packets++;
                        sender_buffer_.push_back(supload_packet);
                    }
                    send_cv_.notify_one();

                    // increments packet index for the next iteration
                    packet_index++;
                }

                // closes file after being done
                sfile.close();
            }
        }
        else
        {
            // if there is no mutex for this file, an error has occured!
            std::string output = get_identifier() + " No file mutex was found for file \"" + file + "\"!";
            aprint(output, 2);
            
            // goes to the next file...
            continue;
        }
    }

    // requests server missing files from session
    for (const std::string& file : files_not_in_current_server) 
    {
        // mounts request packet and adds to buffer
        packet request_packet;
        std::string command = "sdownload|" + file;
        strcharray(command, request_packet.command, sizeof(request_packet.command));

        // adds current file buffer packet to sender buffer
        {
            std::unique_lock<std::mutex> lock(send_mtx_);
            delta_packets++;
            sender_buffer_.push_back(request_packet);
        }
        send_cv_.notify_one();
    }

    output = get_identifier() + " Files now should be updating... A total of"; 
    output += std::to_string(delta_packets) + " were created for this.";
    aprint(output, 2);
}

void ClientSession::client_sent_sdownload_(std::string args, packet buffer, std::string arg2)
{
    // user is sending some file
    std::string file_name = args;
    if(arg2 == "fail")
    {
        // command failed
        std::string output = get_identifier() + " Server requested upload of file \"";
        output += args + "\" failed!";
        aprint(output, 2);
    }
    else
    {
        // user is sending some file
        // server is sending some file
        std::string local_file_path = directory_path_ + args;
        std::string temp_file_path = local_file_path + ".swizdownload";
        
        if(arg2 == "fail")
        {
            std::string output = get_identifier() + " Download request failed!";
            aprint(output, 2);
            return;
        }

        // tries to write on temporary file
        std::ofstream temp_file(temp_file_path, std::ios::app | std::ios::binary);
        if(!temp_file) 
        {
            // given file does not exist locally - informs server
            packet fail_packet;
            std::string command_response = "sdownload|" + args + "|fail";
            strcharray(command_response, fail_packet.command, sizeof(fail_packet.command));
            std::string args_response = "Given file could not be created or accessed";
            args_response += "on user server folder.";
            strcharray(args_response, fail_packet.payload, args_response.size());
            fail_packet.payload_size = args_response.size();

            // adds to sender buffer
            {
                std::unique_lock<std::mutex> lock(send_mtx_);
                sender_buffer_.push_back(fail_packet);
            }
            send_cv_.notify_one();

            std::string output = get_identifier() + " Could not write on file \"";
            output += file_name + "\" sent by user!";
            aprint(output, 2);
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
                if(file_mtx_.find(file_name) != file_mtx_.end())
                {
                    // requests file mutex to change original file
                    std::unique_lock<std::shared_mutex> file_lock(*file_mtx_[file_name]);
                        
                    std::string current_checksum = calculate_md5_checksum(temp_file_path);

                    // deletes temporaty file replacing the original file
                    rename_replacing(temp_file_path, local_file_path);
                    return;
                }
                else
                {
                    std::string output = get_identifier() + " No file mutex was found for \"";
                    output += file_name + "\"!";
                    raise(output, 2);
                }
            }
            return;
        }
    }
}

void ClientSession::client_sent_supload_(std::string args, std::string arg2, char* payload, size_t payload_size, int sequence_number)
{
    // user sent back server file upload request
    // operation failed
    std::string file_name = args;
    if(arg2 == "fail")
    {
        // command failed
        std::string output = get_identifier() + " Server requested download of file \"";
        output += args + "\" failed!";
        aprint(output, 2);
    }
    else if(payload != nullptr)
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

        std::string local_file_path = directory_path_ + "/" + this->get_username() + "/" + args;

        // given path is valid - creates file lock
        if(file_mtx_.find(file_path) == file_mtx_.end())
        {
            auto new_mutex = std::make_shared<std::shared_mutex>();
            file_mtx_.emplace(args, std::move(new_mutex));
        }
        
        // requests newly created file lock
        {
            std::unique_lock<std::shared_mutex> file_lock(*file_mtx_[file_path]);

            std::ofstream file;

            if(sequence_number == 0)
            {
                file = std::ofstream(local_file_path, std::ios::binary);
            }
            else
            {
                file = std::ofstream(local_file_path, std::ios::app | std::ios::binary);
            }

            if(!file.is_open()) 
            {
                std::string output = "Local machine could not acess given file: ";
                output += "\"" + local_file_path + "\"!";
                aprint(output, 3);
                return;
            }
            else
            {      
                // file.seekg(0, std::ios::end);
                file.write(payload, payload_size);

                // closes files after being done
                file.close();

                delete[] payload;
                return;
            }
        }
    }
    else
    {
        // malformed command
        malformed_command_("supload");
    }
}

std::string ClientSession::slist_()
{
    // returns a string list of every file hosted for this session
    DIR* dir = opendir(directory_path_.c_str());
    if(dir == nullptr) 
    {
        std::string output = get_identifier() + " Could not acess user folder!";
        raise(output, 2);
    }

    // main output string
    std::string output = "";
    std::function<void(const std::string&)> list_files_recursively = [&](const std::string& current_path) 
    {

        DIR* dir = opendir(current_path.c_str());
        if(dir == nullptr) 
        {
            std::string output = get_identifier() + " Could not acess \"";
            output += current_path + "\" user folder!";
            raise(output, 2);
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
                    // removes server filesystem prefix from file path
                    std::string current_file_string = file_path;
                    size_t pos = current_file_string.find(directory_path_);
                    if(pos != std::string::npos) 
                    {
                        current_file_string.erase(pos, directory_path_.length());
                    }

                    if(output.size() > 0)
                    {
                        // theres a few files already
                        output += "|" + current_file_string;
                    }
                    else
                    {
                        output += current_file_string;
                    }

                    //output += "\n\t\t\tFile name: " + std::string(entry->d_name);
                    //output += "\n\t\t\tFile path: " + file_path;

                }
                else if(entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) 
                {
                    list_files_recursively(current_path + "/" + entry->d_name);
                }
            }
        }
        closedir(dir);
    };

    // gathers unformatted file list
    list_files_recursively(directory_path_);

    return output;
}
