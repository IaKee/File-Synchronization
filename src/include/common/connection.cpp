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
namespace js = json::json;

Connection::Connection() 
    :   sockfd_(-1) 
    {
        // nothing here
    };


bool Connection::create_server() 
{
        backlog_ = 5;

        struct sockaddr_in serverAddress;
        memset(&serverAddress, 0, sizeof(serverAddress));
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddress.sin_port = htons(port_);

        std::cout << "Endereco do server: " << inet_ntoa(serverAddress.sin_addr) << std::endl;

        if(bind(sockfd_, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
        {
            
            std::cerr << "binding error" << std::endl;
            close(sockfd_);
            return false;
        }

        socklen_t addressLen = sizeof(serverAddress);
        int addr = getsockname(sockfd_, (struct sockaddr*)&serverAddress, &addressLen);
        std::cout << "Endereco do server: " << addr << std::endl;

        if(listen(sockfd_, backlog_) == -1) 
        {
            std::cerr << "Error listening on port" << port_ << std::endl;
            close(sockfd_);
            return false;
        }

        std::cout << "listening on port" << port_ << std::endl;
        return true;
        
}

bool Connection::create_socket() 
{
    // creates a new socket for communication
    // by using SOCK_STREAM, TCP is specified
    std::cout << PROMPT_PREFIX_CLIENT << SOCK_CREATING;
    TTR::Timer timer;

    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    return sockfd_ != -1;
}

void Connection::connect_to_server(const std::string& ip_addr, int port) 
{   
    std::cout << PROMPT_PREFIX_CLIENT << CONNECTING_TO_SERVER;
    std::cout.flush();
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

    
    handle_connection();
    // Now you have a connected socket (sockfd_) that you can use for communication.
}

int Connection::accept_connection() //Usado pelo servidor
{
    struct sockaddr_in client_address;
    socklen_t client_address_length = sizeof(client_address);
    int client_socket = accept(client_socket, (struct sockaddr *)&client_address, &client_address_length);
    if (client_socket == -1) 
    {
        std::cerr << "Error accepting connection." << std::endl;
        return -1;
    }

    return client_socket;
}

void Connection::handle_connection()
{
    const char *message = "message";
    send(sockfd_, message, strlen(message),0);
    
    //...

    //close(sockfd_);
}

void Connection::close_socket() //Usado pelo cliente
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

void Connection::set_port(int port)
{
    port_ = port;
}

std::string Connection::get_address()
{
    return address_;
}

int Connection::get_port()
{
    return port_;
}

//Faz o upload do arquivo para o servidor
void Connection::uploadFile(const std::string& fileToUpload, const std::string& filename, const std::string& username) 
{

    // Open the file to be uploaded
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
}

//Baixa um arquivo do servidor
void Connection::downloadFile(const std::string& filename, const std::string& username) 
{
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
}

void Connection::deleteFile(const std::string& filename, const std::string& username) 
{
        // Create a JSON object with the required keys
        js jsonObject;
        jsonObject["function_type"] = "deleteFile";
        jsonObject["username"] = username;
        jsonObject["filename"] = filename;

        // Convert the JSON object to a string
        std::string jsonData = jsonObject.dump();
        // Send the combined data to the server
        send(sockfd_, jsonData.c_str(), jsonData.size(), 0);
}