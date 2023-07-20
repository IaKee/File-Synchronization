// C++
#include <unistd.h>
#include <cstring>
#include <string>
#include <stdexcept>
#include <iostream>

// connection
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

// locals
#include "connection.hpp"
#include "lang.hpp"
#include "timer.hpp"

// namespace
using namespace connection;

Connection::Connection() 
    :   sockfd(-1) 
    {
        // nothing here
    }

bool Connection::create_socket() 
{
    // creates a new socket for communication
    // by using SOCK_STREAM, TCP is specified
    std::cout << PROMPT_PREFIX << SOCK_CREATING;
    TTR::Timer timer;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    return sockfd != -1;
}

void Connection::connect_to_server(const string& ip_addr, int port) 
{   
    std::cout << PROMPT_PREFIX << CONNECTING_TO_SERVER;
    std::cout.flush();
    TTR::Timer timer;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        timer.cancel();
        throw std::runtime_error(ERROR_SOCK_CREATING);
    }

    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip_addr.c_str(), &server_addr.sin_addr) <= 0) 
    {
        timer.cancel();
        throw std::runtime_error(ERROR_CONVERTING_IP);
    }

    if (connect(sockfd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) 
    {
        timer.cancel();
        throw std::runtime_error(ERROR_CONNECTING_SERVER);
    }

    // Now you have a connected socket (sockfd) that you can use for communication.
}

int Connection::accept_connection() 
{
    // Implementação do método
    return 0;
}

void Connection::close_socket() 
{
    if (sockfd != -1) 
    {
        close(sockfd);
        sockfd = -1;
    }
}

string Connection::get_host_by_name(const string& host_name) 
{   
    std::cout << PROMPT_PREFIX << RESOLVING_HOST;
    std::cout.flush();

    TTR::Timer timer;

    struct hostent* host = gethostbyname(host_name.c_str());
    if (host == nullptr) 
    {
        timer.cancel();
        throw std::runtime_error(ERROR_RESOLVING_HOST);
    }

    char ip[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, host->h_addr, ip, sizeof(ip)) == nullptr) 
    {
        timer.cancel();
        throw std::runtime_error(ERROR_CONVERTING_IP);
    }

    return ip;
}