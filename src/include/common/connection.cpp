// C++
#include <cstring>
#include <string>
#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <functional>

// connection
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

// locals
#include "connection.hpp"
#include "json.hpp"
#include "lang.hpp"
#include "utils.hpp"

// namespace
using namespace connection;
using json = nlohmann::json;

Connection::Connection() 
    :   running_accept_(false),
        backlog_(5),
        timeout_(3)
{
    // nothing here yet
};

Connection::~Connection()
{
    // closes reamining open threads
    if(accept_th_.joinable())
    {
        running_accept_.store(false);
        accept_status_.notify_all();
        accept_th_.join();
    }

    if(recieve_th_.joinable())
    {
        running_recieve_.store(false);
        recieve_status_.notify_all();
        recieve_th_.join();
    }

    if(send_th_.joinable())
    {
        running_send_.store(false);
        send_status_.notify_all();
        send_th_.join();
    }
}
void Connection::create_socket() 
{   
    async_utils::async_print("\t[CONNECTION MANAGER] Creating socket... ");

    // creates a new socket for communication
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);  // SOCK_STREAM specifies TCP
    bool sock_error = (sockfd_ == -1);
    
    if(sock_error)
    {
        throw std::runtime_error("\t[CONNECTION MANAGER] Error creating socket!");
    }
}

void Connection::create_server() 
{
    async_utils::async_print("\t[CONNECTION MANAGER] Creating server...");
    
    // sets socket to passive and listens on given port
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port_);


    if(bind(sockfd_, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        throw std::runtime_error("[CONNECTION MANAGER] Error binding port to socket!");
    }

    socklen_t address_len = sizeof(server_address);
    bool resolved_flag = getsockname(sockfd_, (struct sockaddr*)&server_address, &address_len);
    host_address_ = inet_ntoa(server_address.sin_addr);

    // sets socket to passive mode to listen for client connections
    if(listen(sockfd_, backlog_) == -1) 
    {
        throw std::runtime_error("[CONNECTION MANAGER] Error setting socket to passive!");
    }
}

void Connection::connect_to_server(const std::string& ip_addr, int port) 
{   
    async_utils::async_print("\t[CONNECTION MANAGER] Connecting to server..." + ip_addr + ":" + std::to_string(port));
    
    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if(inet_pton(AF_INET, ip_addr.c_str(), &server_addr.sin_addr) <= 0) 
    {
        throw std::runtime_error("[CONNECTION MANAGER] Error converting IP address!");
    }

    if(connect(sockfd_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) 
    {
        throw std::runtime_error("[CONNECTION MANAGER] Error connecting to server!");
    }

    // sends the username and hostname to server
    //send_username_and_hostname(sockfd_);
}

void Connection::server_accept_loop(
    std::function<void(int, std::string, std::string)> connection_stablished_callback) 
{
    async_utils::async_print("\t[CONNECTION MANAGER] Starting server accept loop...");

    try
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sockfd_, &read_fds);

        int max_fd = sockfd_;

        while(running_accept_.load()) 
        {
            fd_set temp_fds = read_fds;

            timeval read_timeout;
            read_timeout.tv_sec = timeout_;
            read_timeout.tv_usec = 0;

            // waits for updates on the socket or the signal pipe
            int result = select(max_fd + 1, &temp_fds, nullptr, nullptr, &read_timeout);
            
            if(result > 0) 
            {
                if(FD_ISSET(sockfd_, &temp_fds)) 
                {
                    // waits for the new connection
                    int new_socket = accept(sockfd_, nullptr, nullptr);
                    
                    if(new_socket >= 0) 
                    {
                        async_utils::async_print("\t[CONNECTION MANAGER] Got a new connection!");
                        
                        // connection stablished, tries to recieve identification from the new user
                        try
                        {
                            char buffer[1024];
                            recieve_data(buffer, sizeof(buffer), new_socket);
                            
                            // decodes string
                            char* username_token = strtok(buffer, "|");
                            char* machine_name_token = strtok(nullptr, "|");
                            std::string username;
                            std::string machine_name;
                            if(username_token && machine_name_token) 
                            {
                                username = username_token;
                                machine_name = machine_name_token;
                            }
                            else
                            {
                                async_utils::async_print("\t[CONNECTION MANAGER] Connection refused! Invalid user info!");
                                char refuse_buffer[1024] = "logout|invalid_identification";
                                send_data(refuse_buffer, sizeof(refuse_buffer), new_socket);
                                close(new_socket);
                                return;
                            }
                            
                            if(is_valid_username(username))
                            {
                                try
                                {
                                    connection_stablished_callback(new_socket, username, machine_name);
                                    async_utils::async_print("\t[CONNECTION MANAGER] User " + username + " joined with a new session on " + machine_name + "!");
                                }
                                catch(const std::exception& e)
                                {
                                    async_utils::async_print("\t[CONNECTION MANAGER] Could not stablish connection with user: " + std::string(e.what()));
                                    close(new_socket);
                                }
                            }
                            else
                            {
                                async_utils::async_print("\t[CONNECTION MANAGER] Connection refused! Invalid user info!");
                                char refuse_buffer[1024] = "logout|invalid_identification";
                                send_data(refuse_buffer, sizeof(refuse_buffer), new_socket);
                                close(new_socket);
                            }
                        }
                        catch(const std::exception& e)
                        {
                            async_utils::async_print("\t[CONNECTION MANAGER] Exception occured while accepting connection: " + std::string(e.what()));
                            close(new_socket);
                        }
                    }
                }
            }
            else if(result == 0)
            {
                // read timeout
                // nothing happens, tries again on the next iteration
            }
            else
            {
                async_utils::async_print("\t[CONNECTION MANAGER] Select error!");
            }
        }
        async_utils::async_print("\t[CONNECTION MANAGER] Accept loop terminated!");
    }
    catch(const std::exception& e)
    {
        async_utils::async_print("\t[CONNECTION MANAGER] Error occured on server_accept_loop(): " + std::string(e.what()));
        throw std::runtime_error(e.what());
    }
    catch(...)
    {
        async_utils::async_print("\t[CONNECTION MANAGER] Unknown error occured on server_accept_loop()!");
        throw std::runtime_error("[CONNECTION MANAGER] Unknown error occured on server_accept_loop()!");
    }
}

void Connection::close_socket()
{
    async_utils::async_print("\t[CONNECTION MANAGER] Closing socket...");
    
    if (sockfd_ != -1) 
    {
        close(sockfd_);
        sockfd_ = -1;
    }
}

void Connection::start_accepting_connections()
{
    async_utils::async_print("\t[CONNECTION MANAGER] Enabling accept flag...");
    // starts accept loop on another thread
    if(running_accept_.load() == false)
    {
        running_accept_.store(true);

    }
}

void Connection::stop_accepting_connections()
{
    async_utils::async_print("\t[CONNECTION MANAGER] Disabling accept flag...");
    
    // breaks accepting loop
    if(running_accept_.load() == true)
    {
        running_accept_.store(false);
    }
}

std::string Connection::get_host_by_name(const std::string& host_name) 
{   
    async_utils::async_print("\t[CONNECTION MANAGER] Resolving host...");

    struct hostent* host = gethostbyname(host_name.c_str());
    if(host == nullptr) 
    {
        throw std::runtime_error("[CONNECTION MANAGER] Error resolving host!");
    }

    char ip[INET_ADDRSTRLEN];
    if(inet_ntop(AF_INET, host->h_addr, ip, sizeof(ip)) == nullptr) 
    {
        throw std::runtime_error("[CONNECTION MANAGER] Error converting IP address!");
    }

    return ip;
}

void Connection::send_data(char* buffer, std::size_t buffer_size, int sockfd, int timeout)
{
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
        send_timeout.tv_sec = timeout_;
    }
    else
    {
        send_timeout.tv_sec = timeout;
    }

    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(socket, &write_fds);

    int result = select(socket + 1, nullptr, &write_fds, nullptr, &send_timeout);
    
    if(result == -1) 
    {
        throw std::runtime_error("[CONNECTION MANAGER] Error sending buffer on socket " + std::to_string(socket) + "!");
    } 
    else if(result == 0) 
    {
        throw std::runtime_error("[CONNECTION MANAGER] Timed out on socket " + std::to_string(socket) + "!");
    }

    std::size_t total_sent = 0;
    if(FD_ISSET(socket, &write_fds))
    {

        while (total_sent < buffer_size)
        {
            int bytes_sent = send(socket, buffer + total_sent, buffer_size - total_sent, 0);
            
            if (bytes_sent == -1) 
            {
                throw std::runtime_error("[CONNECTION MANAGER] Error sending buffer on socket " + std::to_string(socket) + "!");
            }
            total_sent += bytes_sent;
        }
    }
}

void Connection::recieve_data(char* buffer, std::size_t buffer_size, int sockfd, int timeout)
{
    int socket;
    if(sockfd == -1)
    {
        socket = sockfd_;
    }
    else
    {
        socket = sockfd;
    }

    timeval recieve_timeout;
    recieve_timeout.tv_usec = 0;
    if(timeout == -1)
    {
        recieve_timeout.tv_sec = timeout_;
    }
    else
    {
        recieve_timeout.tv_sec = timeout;
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(socket, &read_fds);

    int result = select(socket + 1, &read_fds, nullptr, nullptr, &recieve_timeout);

    if(result == -1) 
    {
        throw std::runtime_error("[CONNECTION MANAGER] Error recieving buffer on socket " + std::to_string(socket) + "!");
    } 
    else if(result == 0) 
    {
        throw std::runtime_error("[CONNECTION MANAGER] Timed out on socket " + std::to_string(socket) + "!");
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
                throw std::runtime_error("[CONNECTION MANAGER] Connection terminated by remote host!");
            } 
            else 
            {
                throw std::runtime_error("[CONNECTION MANAGER] Error recieving buffer on socket " + std::to_string(socket) + "!");
            }
        }
    }
}

void Connection::upload_file(const std::string& filename, const std::string& username) 
{
    async_utils::async_print("\t[CONNECTION MANAGER] Uploading file \"" + filename + "\"...");
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

int Connection::get_sock_fd()
{
    return sockfd_;
}

std::string Connection::get_address()
{
    return host_address_;
}