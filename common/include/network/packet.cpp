// standard c++
#include <iostream>
#include <cstddef>

// local modules
#include "packet.hpp"
#include "connection_manager.hpp"

using namespace utils_packet;
using namespace connection;

void ConnectionManager::send_packet(const packet& p, int sockfd, int timeout)
{
    char header_buffer[sizeof(packet) - sizeof(char*)];
    
    std::memcpy(header_buffer, &p, sizeof(packet) - sizeof(char*));

    if(log_every_packet)
        aprint(
            "sending a packet of size " 
            + std::to_string(sizeof(header_buffer)) 
            + "b with the command:" 
            + p.command);
    
    send_data(header_buffer, sizeof(header_buffer), sockfd, timeout);

    // sends the payload part of the packet
    if (p.payload_size > 0)
    {
        send_data(p.payload, p.payload_size, sockfd, timeout);
    }
}

void ConnectionManager::receive_packet(packet* p, int sockfd, int timeout)
{
    char header_buffer[sizeof(packet) - sizeof(char*)];

    if(log_every_packet)   
        aprint(
            "expecting a packet of size " 
            + std::to_string(sizeof(header_buffer)) 
            + "b with the command:" 
            + p->command
            + " (this should be null - check for invalid memory references)");

    // tries to receive packet
    receive_data(header_buffer, sizeof(header_buffer), sockfd, timeout);

    std::memcpy(p, header_buffer, sizeof(packet) - sizeof(char*));

    // receives the payload
    if (p->payload_size > 0)
    {
        p->payload = new char[p->payload_size];
        receive_data(p->payload, p->payload_size, sockfd, timeout);
    }
    else
    {
        p->payload = nullptr;
    }
}