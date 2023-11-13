// C++
#include <iostream>
#include <string>
#include <stdexcept>
#include <exception>
#include <unistd.h>

// connection
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

// locals
#include "connection_manager.hpp"
#include "async_cout.hpp"
#include "utils.hpp"
#include "utils_packet.hpp"

// namespace
using namespace connection;
using namespace async_cout;

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

void ConnectionManager::create_socket() 
{   
    aprint("Creating socket... ");

    // creates a new socket for communication
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);  // SOCK_STREAM specifies TCP
    bool sock_error = (sockfd_ == -1);
    
    if(sock_error)
    {
        raise("Error creating socket!");
    }
}

void ConnectionManager::close_socket()
{
    aprint("Closing socket...");
    
    if (sockfd_ != -1) 
    {
        close(sockfd_);
        sockfd_ = -1;
    }
}

void ConnectionManager::send_data(char* buffer, std::size_t buffer_size, int sockfd, int timeout)
{
    if(buffer_size == 0)
    {
        // nothing to send
        return;
    }

    int socket;
    if(sockfd == -1)
    {
        socket = sockfd_;
    }
    else
    {
        socket = sockfd;
    }

    timeval send_timeout;
    send_timeout.tv_usec = 0;
    if(timeout == -1)
    {
        //send_timeout.tv_sec = send_timeout_;
        send_timeout.tv_sec = 3;
    }
    else
    {
        //send_timeout.tv_sec = timeout;
        send_timeout.tv_sec = 3;
    }

    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(socket, &write_fds);

    int result = select(socket + 1, nullptr, &write_fds, nullptr, &send_timeout);
    if(result == -1) 
    {
        raise("Error sending buffer on socket " + std::to_string(socket) + "!");
    } 
    else if(result == 0) 
    {
        raise("Timed out on socket " + std::to_string(socket) + "!");
    }

    std::size_t total_sent = 0;
    if(FD_ISSET(socket, &write_fds))
    {

        while (total_sent < buffer_size)
        {
            int bytes_sent = send(socket, buffer + total_sent, buffer_size - total_sent, 0);
            
            if (bytes_sent == -1) 
            {
                raise("Error sending buffer on socket " + std::to_string(socket) + "!");
            }
            total_sent += bytes_sent;
        }
    }
}

void ConnectionManager::receive_data(char* buffer, std::size_t buffer_size, int sockfd, int timeout)
{
    if(buffer_size == 0)
    {
        // nothing to receive
        return;
    }
    
    int socket;
    if(sockfd == -1)
    {
        socket = sockfd_;
    }
    else
    {
        socket = sockfd;
    }

    timeval receive_timeout;
    receive_timeout.tv_usec = 0;
    if(timeout == -1)
    {
        //receive_timeout.tv_sec = receive_timeout_;
        receive_timeout.tv_sec = 3;
    }
    else
    {
        //receive_timeout.tv_sec = timeout;
        receive_timeout.tv_sec = 3;
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(socket, &read_fds);

    int result = select(socket + 1, &read_fds, nullptr, nullptr, &receive_timeout);

    if(result == -1) 
    {
        raise("Error recieving buffer on socket " + std::to_string(socket) + "!");
    } 
    else if(result == 0) 
    {
        raise("Timed out on socket " + std::to_string(socket) + "!");
    }

    if(FD_ISSET(socket, &read_fds)) 
    {
        ssize_t total_received = 0;
        while (total_received < buffer_size) 
        {
            ssize_t bytes_received = recv(socket, buffer + total_received, buffer_size - total_received, 0);
            
            if(bytes_received > 0) 
            {
                total_received += bytes_received;
            } 
            else if(bytes_received == 0) 
            {
                raise("Connection terminated by remote host!");
            } 
            else 
            {
                raise("Error recieving buffer on socket " + std::to_string(socket) + "!");
            }
        }
    }
}

void ConnectionManager::send_packet(const packet& p, int sockfd, int timeout)
{
    char header_buffer[sizeof(packet) - sizeof(char*)];
    
    std::memcpy(header_buffer, &p, sizeof(packet) - sizeof(char*));

    // TODO: remove this
    aprint("sending a packet of size " + std::to_string(sizeof(header_buffer)) + "b with the command:" + p.command);
    
    send_data(header_buffer, sizeof(header_buffer), sockfd, timeout);

    // sends the payload part of the packet
    if (p.payload_size > 0)
    {
        send_data(p.payload, p.payload_size, sockfd, timeout);
    }
}


void ConnectionManager::receive_packet(packet& p, int sockfd, int timeout)
{
    char header_buffer[sizeof(packet) - sizeof(char*)];
    
    aprint("expecting a packet of size " + std::to_string(sizeof(header_buffer)) + "b with the command:" + p.command);

    // tries to receive packet
    receive_data(header_buffer, sizeof(header_buffer), sockfd, timeout);

    std::memcpy(&p, header_buffer, sizeof(packet) - sizeof(char*));

    // receives the payload
    if (p.payload_size > 0)
    {
        p.payload = new char[p.payload_size];
        receive_data(p.payload, p.payload_size, sockfd, timeout);
    }
    else
    {
        p.payload = nullptr;
    }
}

void connection::aprint(std::string content, int scope, bool endl)
{
    std::string scope_name = "";
    bool valid_scope = false;

    switch (scope)
    {
        case 1:
        {
            scope_name = "CLIENT";
            valid_scope = true;
            break;
        }
        case 2:
        {
            scope_name = "SERVER";
            valid_scope = true;
            break;
        }
        default:
        {
            break;
        }
    }

    if(valid_scope)
        scope_name = " - " + scope_name;

    print(content, "connection manager" + scope_name, 1, 0, 3, endl);
}

void connection::raise(std::string error, int scope)
{
    std::string scope_name = "";
    bool valid_scope = false;

    switch (scope)
    {
        case 1:
        {
            scope_name = "CLIENT";
            valid_scope = true;
            break;
        }
        case 2:
        {
            scope_name = "SERVER";
            valid_scope = true;
            break;
        }
        default:
        {
            break;
        }
    }

    if(valid_scope)
        scope_name = "[" + scope_name + "]";

    std::string exception_string = "[CONNECTION MANAGER]" + scope_name + error;
    throw std::runtime_error(exception_string);
}