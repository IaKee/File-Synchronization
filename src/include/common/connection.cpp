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
#include "json.hpp"
#include "lang.hpp"
#include "timer.hpp"

// namespace
using namespace connection;
using json = nlohmann::json;

Connection::Connection() 
    :   sockfd_(-1),
        backlog_(5),
        signal_pipe_(nullptr),
        running_accept_(true)
{
    // nothing here yet
};

bool Connection::create_socket() 
{
    // creates a new socket for communication
    TTR::Timer timer;

    // by using SOCK_STREAM, TCP is specified
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    bool sock_flag = sockfd_ != -1;

    if(!sock_flag)
        timer.cancel();

    return sock_flag;
}

void Connection::create_server() 
{
    // sets socket to passive and listens on given port
    TTR::Timer timer;

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port_);

    host_address_ = inet_ntoa(server_address.sin_addr);

    if(bind(sockfd_, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        timer.cancel();
        throw std::runtime_error(ERROR_BIND);
    }

    socklen_t address_len = sizeof(server_address);
    int addr = getsockname(sockfd_, (struct sockaddr*)&server_address, &address_len);

    // sets socket to passive mode to listen for client connections
    if (listen(sockfd_, backlog_) == -1) 
    {
        throw std::runtime_error(ERROR_LISTEN);
    }
}

void Connection::connect_to_server(const std::string& ip_addr, int port) 
{   
    TTR::Timer timer;

    int sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0) 
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

    if (connect(sockfd_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) 
    {
        timer.cancel();
        throw std::runtime_error(ERROR_CONNECTING_SERVER);
    }

    
    //handle_connection();
    // Now you have a connected socket (sockfd_) that you can use for communication.
}


void Connection::start_accepting_connections() 
{
    try
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sockfd_, &read_fds);
        FD_SET(signal_pipe_[0], &read_fds);

        int max_fd = std::max(sockfd_, signal_pipe_[0]);
        std::cout << "passou" << std::endl;

        while (running_accept_.load()) 
        {
            fd_set temp_fds = read_fds;

            // waits for updates on the socket or the signal pipe
            int result = select(max_fd + 1, &temp_fds, nullptr, nullptr, nullptr);
            
            if (result > 0) 
            {
                if (FD_ISSET(sockfd_, &temp_fds)) 
                {
                    // waits for the new connection
                    int new_socket = accept(sockfd_, nullptr, nullptr);
                    if (new_socket >= 0) 
                    {
                        // new connection stablished
                        // TODO:: treat here
                        close(new_socket);  // remove this
                    }
                }

                // checks for pipe signal to stop accepting connections
                if (FD_ISSET(signal_pipe_[0], &temp_fds)) 
                {
                    break;
                }
            }
        }

        // closes the reading end of the signal pipe
        close(signal_pipe_[0]); 
    }
    catch(const std::exception& e)
    {
        std::cerr << "[ERROR][CONNECTION] Error occured on start_accepting_connections(): " << e.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "[ERROR][CONNECTION] Unknown error occured on start_accepting_connections()!" << std::endl;
    }
}

void Connection::stop_accepting_connections()
{
    // breaks accepting loop
    running_accept_.store(false);
}

void Connection::close_socket()
{
    if (sockfd_ != -1) 
    {
        close(sockfd_);
        sockfd_ = -1;
    }
}

std::string Connection::get_host_by_name(const std::string& host_name) 
{   
    std::cout << PROMPT_PREFIX_CLIENT << RESOLVING_HOST;
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

void Connection::upload_file(const std::string& filename, const std::string& username) 
{
    // uploads a given file

    /*
    std::ifstream fileStream(fileToUpload, std::ios::binary);
    if (!fileStream) {
        std::cerr << "Error opening file: " << fileToUpload << std::endl;
        return;
    }

    // Read the entire file content into a string
    std::stringstream fileContentStream;
    fileContentStream << fileStream.rdbuf();
    std::string fileContent = fileContentStream.str();

    // Create a JSON object with the required keys
    js jsonObject;
    jsonObject["function_type"] = "uploadFile";
    jsonObject["username"] = username;
    jsonObject["filename"] = filename;
    jsonObject["file_binary"] = fileContent

    // Convert the JSON object to a string
    std::string jsonData = jsonObject.dump();

    // Send the combined data to the server
    send(sockfd_, jsonData.c_str(), jsonData.size(), 0);
    fileStream.close();
    */
}

void Connection::download_file(const std::string& filename, const std::string& username) 
{
    // downloads a given file

    /*
    // Create a JSON object with the required keys
    js jsonObject;
    jsonObject["function_type"] = "downloadFile";
    jsonObject["username"] = username;
    jsonObject["filename"] = filename;

    // Convert the JSON object to a string
    std::string jsonData = jsonObject.dump();
    // Send the combined data to the server
    send(sockfd_, jsonData.c_str(), jsonData.size(), 0);    

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Erro ao criar o arquivo local." << std::endl;
        return;
    }

    int file_size;
    recv(sockfd_, &file_size, sizeof(file_size), 0);

    char buffer[1024];
    int bytes_received = 0;
    while (bytes_received < file_size) {
        int bytes = recv(sockfd_, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            break;
        }
        file.write(buffer, bytes);
        bytes_received += bytes;
    }

    file.close();

    if (bytes_received == file_size) {
        std::cout << "Download concluído: " << filename << std::endl;
    } else {
        std::cerr << "Erro ao receber o arquivo." << std::endl;
    }
    */
}

void Connection::set_port(int port)
{
    port_ = port;
}

int Connection::get_port()
{
    return port_;
}

std::string Connection::get_address()
{
    return host_address_;
}

void Connection::link_pipe(int* pipe)
{   
    // links signal pipe with server application
    signal_pipe_ = pipe;
}