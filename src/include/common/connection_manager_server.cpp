// standard c++
#include <functional>

// local includes
#include "connection_manager.hpp"
#include "async_cout.hpp"
#include "utils.hpp"
#include "utils_packet.hpp"

using namespace connection;
using namespace async_cout;

ServerConnectionManager::ServerConnectionManager()
{
    // empty initializer
}

ServerConnectionManager::~ServerConnectionManager()
{
    // close remainging open threads
    if(accept_th_.joinable())
    {
        running_accept_.store(false);
        accept_status_.notify_all();
        accept_th_.join();
    }
}

void ServerConnectionManager::create_server() 
{
    aprint("\t[SERVER CONNECTION MANAGER] Creating server...");
    
    // sets socket to passive and listens on given port
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(this->get_port());

    if(bind(
        this->get_sock_fd(), 
        (struct sockaddr *)&server_address, 
        sizeof(server_address)) == -1)
    {
        throw std::runtime_error("[SERVER CONNECTION MANAGER] Error binding port to socket!");
    }

    socklen_t address_len = sizeof(server_address);
    bool resolved_flag = getsockname(this->get_sock_fd(), (struct sockaddr*)&server_address, &address_len);
    set_address(inet_ntoa(server_address.sin_addr));

    // sets socket to passive mode to listen for client connections
    if(listen(this->get_sock_fd(), this->get_backlog()) == -1) 
    {
        throw std::runtime_error("[SERVER CONNECTION MANAGER] Error setting socket to passive!");
    }
}

void ServerConnectionManager::start_accept_loop()
{
    aprint("\t[SERVER CONNECTION MANAGER] Enabling accept flag...");
    // starts accept loop on another thread
    if(running_accept_.load() == false)
    {
        running_accept_.store(true);
    }
    else
    {
        aprint("\t[SERVER CONNECTION MANAGER] Tried to start accept loop which is already running!");
    }
}

void ServerConnectionManager::stop_accept_loop()
{
    aprint("\t[SERVER CONNECTION MANAGER] Disabling accept flag...");
    
    // breaks accepting loop
    if(running_accept_.load() == true)
    {
        running_accept_.store(false);
    }
    else
    {
        aprint("\t[SERVER CONNECTION MANAGER] Tried to stop accept loop without ever running!");
    }
}

void ServerConnectionManager::server_accept_loop(
    std::function<void(int, std::string, std::string)> connection_stablished_callback) 
{
    aprint("\t[SERVER CONNECTION MANAGER] Starting server accept loop...");

    try
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(this->get_sock_fd(), &read_fds);

        int max_fd = this->get_sock_fd();

        while(running_accept_.load()) 
        {
            fd_set temp_fds = read_fds;

            timeval read_timeout;
            read_timeout.tv_sec = this->get_receive_timeout();
            read_timeout.tv_usec = 0;
            
            // waits for updates on the socket or the signal pipe
            int result = select(max_fd + 1, &temp_fds, nullptr, nullptr, &read_timeout);
            
            if(result > 0) 
            {
                if(FD_ISSET(this->get_sock_fd(), &temp_fds)) 
                {
                    // waits for the new connection
                    int new_socket = accept(this->get_sock_fd(), nullptr, nullptr);
                    
                    if(new_socket >= 0) 
                    {
                        aprint("\t[SERVER CONNECTION MANAGER] Got a new connection request...");
                        
                        try
                        {
                            // connection stablished, tries to recieve identification from the new user
                            packet accept_packet;
                            {
                                std::unique_lock<std::mutex> lock(receive_mtx_);
                                receive_packet(accept_packet, new_socket);
                            }
                            
                            // decodes string into argument list
                            std::vector<std::string> sanitized_payload = split_buffer(accept_packet.command);

                            // verifies argument number
                            if(sanitized_payload.size() != 3)
                            {
                                std::string output = "\t[SERVER CONNECTION MANAGER] Connection refused!";
                                output += " Invalid argument number! Sending refuse packet...";
                                aprint(output);
                                
                                packet refusal_packet;
                                    
                                // fills packet command
                                std::string command_response = "login|fail";
                                strcharray(command_response, refusal_packet.command, sizeof(refusal_packet.command));
                                    
                                // fills packet payload
                                std::string args_response = "Malformed login command, invalid argument number";
                                strcharray(args_response, refusal_packet.payload, args_response.size());
                                refusal_packet.payload_size = args_response.size();
                                    
                                {
                                    std::unique_lock<std::mutex> lock(send_mtx_);
                                    this->send_packet(refusal_packet, new_socket);
                                }

                                close(new_socket);

                                // goes to the next loop iteraction
                                continue;
                            }

                            // while having the correct argument number
                            // decodes them and ensures validity
                            std::string command = sanitized_payload[0];
                            std::string username = sanitized_payload[1];
                            std::string machine_name = sanitized_payload[2];
                            bool valid_connection = is_valid_username(username);

                            // tries to approve connection, after validating
                            if(valid_connection)
                            {
                                try
                                {
                                    // adds new session to internal session vector
                                    connection_stablished_callback(new_socket, username, machine_name);

                                    // mounts and sends a packet approving the login request
                                    packet approval_packet;
                                    std::string command_response = "login|ok|" + new_socket;
                                    strcharray(command_response, approval_packet.command, sizeof(approval_packet.command));
                                    
                                    {
                                        std::unique_lock<std::mutex> lock(send_mtx_);
                                        this->send_packet(approval_packet, new_socket);
                                    }

                                    std::string output = "\t[SERVER CONNECTION MANAGER] User " + username;
                                    output += " joined with a new session on " + machine_name + "!";
                                    aprint(output);

                                    // goes to the next loop iteraction
                                    continue;
                                }
                                catch(const std::exception& e)
                                {
                                    std::string output = "\t[SERVER CONNECTION MANAGER] Could not stablish connection with user: ";
                                    output += std::string(e.what());
                                    aprint(output);

                                    packet refusal_packet;
                                    std::string command_response = "login|fail";
                                    strcharray(command_response, refusal_packet.command, sizeof(refusal_packet.command));
                                    
                                    std::string args_response = "Exception ocurred registring new user:";
                                    args_response += std::string(e.what());
                                    strcharray(args_response, refusal_packet.payload, args_response.size());
                                    refusal_packet.payload_size = args_response.size();
                                    
                                    {
                                        std::unique_lock<std::mutex> lock(send_mtx_);
                                        this->send_packet(refusal_packet, new_socket);
                                    }
                                    
                                    close(new_socket);

                                    // goes to the next loop iteraction
                                    continue;
                                }
                            }
                            else
                            {
                                std::string output = "\t[SERVER CONNECTION MANAGER] Could not stablish connection with user: ";
                                output += "Invalid user info!";
                                aprint(output);

                                packet refusal_packet;
                                std::string command_response = "login|fail";
                                strcharray(command_response, refusal_packet.command, sizeof(refusal_packet.command));
                                std::string args_response = "Invalid user info!";
                                strcharray(args_response, refusal_packet.payload, args_response.size());
                                refusal_packet.payload_size = args_response.size();
                                this->send_packet(refusal_packet, new_socket);

                                close(new_socket);

                                // goes to the next loop iteraction
                                continue;
                            }
                        }
                        catch(const std::exception& e)
                        {
                            std::string output = "\t[SERVER CONNECTION MANAGER] Exception occured while accepting connection: ";
                            output += "\n\t\t" + std::string(e.what());
                            aprint(output);

                            try
                            {
                                close(new_socket);
                            }
                            catch(const std::exception& e)
                            {
                                aprint("\t[SERVER CONNECTION MANAGER] Could not refuse connection. Its gone!");
                            }
                            
                            // goes to the next loop iteraction
                            continue;
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
                aprint("\t[SERVER CONNECTION MANAGER] Select reading error!");
            }
        }
        aprint("\t[SERVER CONNECTION MANAGER] Accept loop terminated.");
    }
    catch(const std::exception& e)
    {
        std::string output = "\t[SERVER CONNECTION MANAGER] Error occured on server accept loop:";
        output += "\n\t\t" + std::string(e.what());
        aprint(output);
        throw std::runtime_error(e.what());
    }
    catch(...)
    {
        throw std::runtime_error("[SERVER CONNECTION MANAGER] Unknown error occured on server accept loop!");
    }
}

void ServerConnectionManager::add_backup(std::string address, int port)
{
    server_backups_address_.push_back(std::make_pair(address, port));
}