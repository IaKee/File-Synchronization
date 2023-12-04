#include "connection_manager.hpp"
#include <functional>

using namespace connection;

void ConnectionManager::create_socket()
{
    aprint("Creating socket... ");

    // creates a new socket for communication
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0); // SOCK_STREAM specifies TCP
    bool sock_error = (sockfd_ == -1);
    
    if (sock_error)
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

void ConnectionManager::send_data(char *buffer, std::size_t buffer_size, int sockfd, int timeout)
{
    if (buffer_size == 0)
    {
        // nothing to send
        return;
    }

    int socket;
    if (sockfd == -1)
    {
        socket = sockfd_;
    }
    else
    {
        socket = sockfd;
    }

    std::size_t total_sent = 0;
    while (total_sent < buffer_size)
    {
        if(log_every_packet) aprint("[Thread " + std::to_string(std::hash<std::thread::id> {} (std::this_thread::get_id())) + "] Trying to send to socket " + std::to_string(socket), 0);
        int bytes_sent = send(socket, buffer + total_sent, buffer_size - total_sent, 0);
        if(log_every_packet) aprint("Sent to socket " + std::to_string(socket) + " " + std::to_string(bytes_sent) + "bytes.", 0);

        if (bytes_sent == -1)
        {
            raise("Error sending buffer on socket " + std::to_string(socket) + "!");
        }
        total_sent += bytes_sent;
    }
}

void ConnectionManager::receive_data(char *buffer, std::size_t buffer_size, int sockfd, int timeout)
{
    if (buffer_size == 0)
    {
        // nothing to receive
        return;
    }

    int socket;
    if (sockfd == -1)
    {
        socket = sockfd_;
    }
    else
    {
        socket = sockfd;
    }
    
    ssize_t total_received = 0;
    while (total_received < buffer_size)
    {
        if(log_every_packet) aprint("[Thread " + std::to_string(std::hash<std::thread::id> {} (std::this_thread::get_id())) + "] Trying to receive to socket " + std::to_string(socket), 0);
        ssize_t bytes_received = recv(socket, buffer + total_received, buffer_size - total_received, 0);
        if(log_every_packet) aprint("Received from socket " + std::to_string(socket) + " " + std::to_string(bytes_received) + "bytes.", 0);

        if (bytes_received > 0)
        {
            total_received += bytes_received;
        }
        else if (bytes_received == 0)
        {
            raise("Connection terminated by remote host!");
        }
        else
        {
            raise("Error recieving buffer on socket " + std::to_string(socket) + "!");
        }
    }
}