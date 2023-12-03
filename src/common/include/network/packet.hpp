# pragma once

#include <iostream>
#include <cstddef>
#include <cstring>

namespace utils_packet
{
    typedef struct packet
    {
        char command[1024];
        int sequence_number = 0;
        std::size_t payload_size = 0; 
        std::size_t expected_packets = 0; // number of expected packets
        char* payload = nullptr;


        packet() : sequence_number(0), payload_size(0), expected_packets(0), payload(nullptr)
        {
            std::fill(std::begin(command), std::end(command), '\0');
        }
    } packet; 
}