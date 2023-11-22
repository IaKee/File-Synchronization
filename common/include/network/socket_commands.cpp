#include "connection_manager.hpp"

using namespace connection;

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