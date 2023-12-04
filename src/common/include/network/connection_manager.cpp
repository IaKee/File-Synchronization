#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

// locals
#include "connection_manager.hpp"

// namespace
using namespace connection;

ConnectionManager::ConnectionManager() 
    :   backlog_(5),
        send_timeout_(3),
        receive_timeout_(3)
{
};

ConnectionManager::~ConnectionManager()
{
    // closes reamining open threads
    if(receive_th_.joinable())
    {
        running_receive_.store(false);
        receive_status_.notify_all();
        receive_th_.join();
    }

    if(send_th_.joinable())
    {
        running_send_.store(false);
        send_status_.notify_all();
        send_th_.join();
    }
    
}

bool ConnectionManager::is_port_available(int port) 
{
    // creates a socket using TCP
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if(sockfd == -1) 
    {
        // Erro na criação do socket
        raise("Error creating socket!");
        return false;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // tries linking socket to port
    int result = bind(sockfd, (struct sockaddr*)&address, sizeof(address));
    
    if(result == 0) 
    {
        // port is available
        close(sockfd);
        return true;
    } 
    else 
    {
        // port is busy
        close(sockfd);
        return false;
    }
}

int ConnectionManager::get_port()
{
    return port_;
}

int ConnectionManager::get_sock_fd()
{
    return sockfd_;
}

int ConnectionManager::get_backlog()
{
    return backlog_;
}

int ConnectionManager::get_receive_timeout()
{
    return receive_timeout_;
}

int ConnectionManager::get_send_timeout()
{
    return send_timeout_;
}

std::string ConnectionManager::get_address()
{
    return host_address_;
}

void ConnectionManager::set_address(std::string address)
{
    host_address_ = address;
}

std::string ConnectionManager::get_host_by_name(const std::string host_name) 
{   
    aprint("Resolving host...");

    struct hostent* host = gethostbyname(host_name.c_str());
    if(host == nullptr) 
    {
        raise("Error resolving host!");
    }

    char ip[INET_ADDRSTRLEN];
    if(inet_ntop(AF_INET, host->h_addr, ip, sizeof(ip)) == nullptr) 
    {
        raise("Error converting IP address!");
    }

    return ip;
}

void ConnectionManager::set_port(int port)
{
    port_ = port;
}