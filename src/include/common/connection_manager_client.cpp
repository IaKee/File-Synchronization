#include <iostream>

#include "utils.hpp"
#include "utils_packet.hpp"
#include "connection_manager.hpp"

using namespace connection;
using namespace utils_packet;

ClientConnectionManager::ClientConnectionManager()
    :   is_connected_(false)
{

}

void ClientConnectionManager::connect_to_server(std::string address, int port) 
{   
    async_utils::async_print("\t[CLIENT CONNECTION MANAGER] Resolving host name...");
    std::string adjusted_address = this->get_host_by_name(address);
    this->set_address(adjusted_address);
    this->set_port(port);

    std::string ip_addr = this->get_address();
    async_utils::async_print("\t[CLIENT CONNECTION MANAGER] Connecting to server..." 
        + ip_addr
        + ":" 
        + std::to_string(port));
    
    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if(inet_pton(AF_INET, ip_addr.c_str(), &server_addr.sin_addr) <= 0) 
    {
        throw std::runtime_error("[CLIENT CONNECTION MANAGER] Error converting IP address!");
    }

    int sockfd = this->get_sock_fd();
    if(connect(
        sockfd, 
        reinterpret_cast<sockaddr*>(&server_addr), 
        sizeof(server_addr)) < 0) 
    {
        throw std::runtime_error("[CLIENT CONNECTION MANAGER] Error connecting to server!");
    }
}

int ClientConnectionManager::login(std::string username, std::string machine_name)
{
    username_ = username;
    machine_name_ = machine_name;

    try
    {
        // mounts and sends login string
        std::string login_command = "login|" + username + "|" + machine_name;

        packet login_packet;
        strcharray(login_command, login_packet.command, sizeof(login_packet.command));
        this->send_packet(login_packet, get_sock_fd());
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error("[CLIENT CONNECTION MANAGER] Could not send login request: \n\t\t" + std::string(e.what()));
    }
    
    try
    {
        // receives and unpacks login confirmation
        packet login_confirmation;
        this->receive_packet(login_confirmation, get_sock_fd());

        std::vector<std::string> sanitized_payload = split_buffer(login_confirmation.payload);
        
        if(sanitized_payload[0] == "login" && sanitized_payload[1] == "ok")
        {
            async_utils::async_print("[CLIENT CONNECTION MANAGER] Login approved!");
            return std::stoi(sanitized_payload[2]);;
        }
        else
        {
            std::string login_response(login_confirmation.payload, login_confirmation.payload_size);
            throw std::runtime_error("[CLIENT CONNECTION MANAGER] Malformed login response: " + login_response);
        }
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error("[CLIENT CONNECTION MANAGER] Login was not approved: \n\t\t" + std::string(e.what()));
    }
    
}