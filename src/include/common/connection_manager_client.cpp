#include <iostream>

#include "async_cout.hpp"
#include "utils.hpp"
#include "utils_packet.hpp"
#include "connection_manager.hpp"

using namespace connection;
using namespace utils_packet;
using namespace async_cout;

ClientConnectionManager::ClientConnectionManager()
    :   is_connected_(false)
{
    // initialization here
}

void ClientConnectionManager::connect_to_server(std::string address, int port) 
{   
    aprint("Resolving host name...", 1);
    std::string adjusted_address = this->get_host_by_name(address);
    this->set_address(adjusted_address);
    this->set_port(port);

    std::string ip_addr = this->get_address();
    aprint("Connecting to server at" + ip_addr + "(" + std::to_string(port) + ")...", 1);
    
    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if(inet_pton(AF_INET, ip_addr.c_str(), &server_addr.sin_addr) <= 0) 
    {
        raise("Error converting IP address!", 1);
    }

    int sockfd = this->get_sock_fd();
    if(connect(
        sockfd, 
        reinterpret_cast<sockaddr*>(&server_addr), 
        sizeof(server_addr)) < 0) 
    {
        raise("Error connecting to server!", 1);
    }
}

int ClientConnectionManager::login(std::string username, std::string machine_name)
{
    username_ = username;
    machine_name_ = machine_name;

    try
    {
        // mounts and sends login command packet
        std::string login_command = "login|" + username + "|" + machine_name;
        packet login_packet;
        strcharray(login_command, login_packet.command, sizeof(login_packet.command));
        
        {
            std::unique_lock<std::mutex> lock(send_mtx_);
            this->send_packet(login_packet, get_sock_fd());
        }
    }
    catch(const std::exception& e)
    {
        std::string eoutput = "Could not send login request:\n\t\t" + std::string(e.what());
        raise(eoutput, 1);
    }
    
    try
    {
        // receives and unpacks login confirmation
        packet login_confirmation;
        {
            std::unique_lock<std::mutex> lock(receive_mtx_);
            this->receive_packet(login_confirmation, get_sock_fd());
        }

        std::vector<std::string> sanitized_payload = split_buffer(login_confirmation.command);
        std::string command = sanitized_payload[0];
        std::string status = sanitized_payload[1];

        if(command == "login")
        {
            if(status == "ok")
            {
                std::string session_sid = sanitized_payload[2];
                int session_id = std::stoi(session_sid);
                aprint("Login approved on session " + session_sid + "!", 1);
                return session_id;
            }
            else if(status == "fail")
            {
                std::string reason = charraystr(login_confirmation.payload, login_confirmation.payload_size);
                raise("Login refused! " + reason, 1);
            }
            else
            {
                raise("Unknown login command status!", 1);
            }
        }
        else        
        {
            std::string eoutput = "Unknown command! Expected \'login\', but got \'" + command + "\'!";
            raise(eoutput, 1);
        }
    }
    catch(const std::exception& e)
    {
        std::string eoutput = "A critical error occured while loggin in: " + std::string(e.what());
        raise(eoutput, 1);
    }

    // unreachable
    return -1;
}