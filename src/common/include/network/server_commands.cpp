// standard c++
#include <functional>

// local includes
#include "connection_manager.hpp"
#include "packet.hpp"
#include "../asyncio/async_cout.hpp"
#include "../utils.hpp"

using namespace connection;
using namespace async_cout;

ServerConnectionManager::ServerConnectionManager()
{
    // empty initializer
}

ServerConnectionManager::~ServerConnectionManager()
{
    // close remainging open threads
    if (accept_th_.joinable())
    {
        running_accept_.store(false);
        accept_status_.notify_all();
        accept_th_.join();
    }
}

void ServerConnectionManager::sconnect_to_server(std::string address, int port) 
{   
    aprint("Resolving host name...", 1);
    std::string adjusted_address = this->get_host_by_name(address);
    this->set_address(adjusted_address);
    this->set_port(port);

    std::string ip_addr = this->get_address();
    aprint("Connecting to server at " + ip_addr + "(" + std::to_string(port) + ")", 1);
    
    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if(inet_pton(AF_INET, ip_addr.c_str(), &server_addr.sin_addr) <= 0) 
    {
        raise("Error converting IP address!", 1);
    }

    int sockfd = this->get_sock_fd();
    if(connect(
        sockfd, 
        reinterpret_cast<sockaddr*>(&server_addr), 
        sizeof(server_addr)) < 0) 
    {
        raise("Error connecting to server!", 1);
    }
}

int ServerConnectionManager::slogin(std::string username, std::string machine_name)
{
    try
    {
        // mounts and sends login command packet
        std::string login_command = "login|" + username + "|" + machine_name;
        packet login_packet;
        strcharray(login_command, login_packet.command, sizeof(login_packet.command));
        
        {
            std::unique_lock<std::mutex> lock(send_mtx_);
            this->send_packet(login_packet, get_sock_fd());
        }
    }
    catch(const std::exception& e)
    {
        std::string eoutput = "Could not send login request:\n\t\t" + std::string(e.what());
        raise(eoutput, 1);
    }
    
    try
    {
        // receives and unpacks login confirmation
        packet login_confirmation;
        {
            std::unique_lock<std::mutex> lock(receive_mtx_);
            this->receive_packet(&login_confirmation, get_sock_fd());
        }

        std::vector<std::string> sanitized_payload = split_buffer(login_confirmation.command);
        std::string command = sanitized_payload[0];
        std::string status = sanitized_payload[1];

        if(command == "login")
        {
            if(status == "ok")
            {
                std::string session_sid = sanitized_payload[2];
                int session_id = std::stoi(session_sid);
                aprint("Login approved on session " + session_sid + "!", 1);
                return session_id;
            }
            else if(status == "fail")
            {
                std::string reason = charraystr(login_confirmation.payload, login_confirmation.payload_size);
                raise("Login refused! " + reason, 1);
            }
            else
            {
                raise("Unknown login command status!", 1);
            }
        }
        else        
        {
            std::string eoutput = "Unknown command! Expected \'login\', but got \'" + command + "\'!";
            raise(eoutput, 1);
        }
    }
    catch(const std::exception& e)
    {
        std::string eoutput = "A critical error occured while loggin in: " + std::string(e.what());
        raise(eoutput, 1);
    }

    // unreachable
    return -1;
}

void ServerConnectionManager::open_server()
{
    aprint("Creating server...", 2);

    // sets socket to passive and listens on given port
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, this->get_address().c_str(), &(server_address.sin_addr));
    server_address.sin_port = htons(this->get_port());

    if (bind(
            this->get_sock_fd(),
            (struct sockaddr *)&server_address,
            sizeof(server_address)) == -1)
    {
        throw std::runtime_error("[SERVER CONNECTION MANAGER] Error binding port to socket!");
    }

    socklen_t address_len = sizeof(server_address);
    bool resolved_flag = getsockname(this->get_sock_fd(), (struct sockaddr *)&server_address, &address_len);
    set_address(inet_ntoa(server_address.sin_addr));

    // sets socket to passive mode to listen for client connections
    if (listen(this->get_sock_fd(), this->get_backlog()) == -1)
    {
        throw std::runtime_error("[SERVER CONNECTION MANAGER] Error setting socket to passive!");
    }
}

void ServerConnectionManager::start_accept_loop()
{
    aprint("Enabling accept flag...", 2);
    // starts accept loop on another thread
    if (running_accept_.load() == false)
    {
        running_accept_.store(true);
    }
    else
    {
        aprint("Tried to start accept loop which is already running!", 2);
    }
}

void ServerConnectionManager::stop_accept_loop()
{
    aprint("Disabling accept flag...", 2);

    // breaks accepting loop
    if (running_accept_.load() == true)
    {
        running_accept_.store(false);
    }
    else
    {
        aprint("Tried to stop accept loop without ever running!", 2);
    }
}

void ServerConnectionManager::server_accept_loop(
    std::function<void(int, std::string, std::string)> connection_stablished_callback,
    std::function<void(int, std::string, std::string)> connection_stablished_callback2)
{
    aprint("Starting server accept loop...", 2);

    try
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(this->get_sock_fd(), &read_fds);

        int max_fd = this->get_sock_fd();

        while (running_accept_.load())
        {
            fd_set temp_fds = read_fds;

            timeval read_timeout;
            read_timeout.tv_sec = this->get_receive_timeout();
            read_timeout.tv_usec = 0;

            // waits for updates on the socket or the signal pipe
            int result = select(max_fd + 1, &temp_fds, nullptr, nullptr, &read_timeout);

            if (result > 0)
            {
                if (FD_ISSET(this->get_sock_fd(), &temp_fds))
                {
                    // waits for the new connection
                    int new_socket = accept(this->get_sock_fd(), nullptr, nullptr);

                    if (new_socket >= 0)
                    {
                        aprint("Got a new connection request...", 2);

                        try
                        {
                            // connection stablished, tries to recieve identification from the new user
                            packet accept_packet;
                            {
                                std::unique_lock<std::mutex> lock(receive_mtx_);
                                receive_packet(&accept_packet, new_socket);
                            }

                            // decodes string into argument list
                            std::vector<std::string> sanitized_payload = split_buffer(accept_packet.command);

                            // verifies argument number
                            if (sanitized_payload.size() != 3)
                            {
                                std::string output = "Connection refused! Invalid argument number!";
                                output += "Sending refusal packet...";
                                aprint(output, 2);

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
                            if (valid_connection)
                            {
                                try
                                {
                                    // adds new session to internal session vector
                                    connection_stablished_callback(new_socket, username, machine_name);

                                    // mounts and sends a packet approving the login request
                                    // packet approval_packet;
                                    // std::string command_response = "login|ok|" + new_socket;
                                    // strcharray(command_response, approval_packet.command, sizeof(approval_packet.command));

                                    // {
                                    //     std::unique_lock<std::mutex> lock(send_mtx_);
                                    //     this->send_packet(approval_packet, new_socket);
                                    // }

                                    std::string output = "User " + username + " joined with a new session on ";
                                    output += machine_name + "!";
                                    aprint(output, 2);

                                    // goes to the next loop iteraction
                                    continue;
                                }
                                catch (const std::exception &e)
                                {
                                    std::string output = "Could not stablish connection with user: " + std::string(e.what());
                                    aprint(output, 2);

                                    packet refusal_packet;
                                    std::string command_response = "login|fail";
                                    strcharray(command_response, refusal_packet.command, sizeof(refusal_packet.command));

                                    std::string args_response = "Exception ocurred registring new user:";
                                    args_response += std::string(e.what());
                                    refusal_packet.payload = new char[args_response.size()+1];
                                    strcharray(args_response, refusal_packet.payload, args_response.size()+1);
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
                                std::string output = "Could not stablish connection with user: Invalid user info!";
                                aprint(output, 2);

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
                        catch (const std::exception &e)
                        {
                            std::string output = "Exception occured while accepting connection\n\t\t";
                            output += std::string(e.what());
                            aprint(output, 2);

                            try
                            {
                                close(new_socket);
                            }
                            catch (const std::exception &e)
                            {
                                aprint("Could not refuse connection. Its gone!", 2);
                            }

                            // goes to the next loop iteraction
                            continue;
                        }
                    }
                }
            }
            else if (result == 0)
            {
                // read timeout
                // no new connection was found
                // goes to the next iteration
            }
            else
            {
                aprint("Select reading error!", 2);
            }
        }
        aprint("Accept loop terminated.", 2);
    }
    catch (const std::exception &e)
    {
        std::string eoutput = "Error occured on server accept loop:\n\t\t" + std::string(e.what());
        aprint(eoutput, 2);
        raise(eoutput, 2);
    }
    catch (...)
    {
        raise("Unknown error occured on server accept loop!", 2);
    }
}

void ServerConnectionManager::server_accept_loop(
    std::function<void(int, std::string, std::string)> connection_stablished_callback)
{   
    aprint("Starting front_end accept loop...", 2);

    try
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(this->get_sock_fd(), &read_fds);

        int max_fd = this->get_sock_fd();

        while (running_accept_.load())
        {
            fd_set temp_fds = read_fds;

            timeval read_timeout;
            read_timeout.tv_sec = this->get_receive_timeout();
            read_timeout.tv_usec = 0;

            // waits for updates on the socket or the signal pipe
            int result = select(max_fd + 1, &temp_fds, nullptr, nullptr, &read_timeout);

            if (result > 0)
            {
                if (FD_ISSET(this->get_sock_fd(), &temp_fds))
                {
                    // waits for the new connection
                    int new_socket = accept(this->get_sock_fd(), nullptr, nullptr);

                    if (new_socket >= 0)
                    {
                        aprint("Got a new connection request...", 2);

                        try
                        {
                            // connection stablished, tries to recieve identification from the new user
                            packet accept_packet;
                            {
                                std::unique_lock<std::mutex> lock(receive_mtx_);
                                receive_packet(&accept_packet, new_socket);
                            }

                            // decodes string into argument list
                            std::vector<std::string> sanitized_payload = split_buffer(accept_packet.command);
                            std::string command = sanitized_payload[0];
                            std::string username = sanitized_payload[1];
                            std::string machine_name = sanitized_payload[2];

                            try
                            {
                                connection_stablished_callback(new_socket, username, machine_name);

                                std::string output = "User " + username + " joined with a new session on ";
                                output += machine_name + "!";
                                aprint(output, 2);

                                send_packet(accept_packet, get_coordinator());

                                // goes to the next loop iteraction
                                continue;
                            }
                            catch (const std::exception &e)
                            {
                                std::string output = "Could not stablish connection with user: " + std::string(e.what());
                                aprint(output, 2);

                                packet refusal_packet;
                                std::string command_response = "login|fail";
                                strcharray(command_response, refusal_packet.command, sizeof(refusal_packet.command));

                                std::string args_response = "Exception ocurred registring new user:";
                                args_response += std::string(e.what());
                                refusal_packet.payload = new char[args_response.size()+1];
                                strcharray(args_response, refusal_packet.payload, args_response.size()+1);
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
                        catch (const std::exception &e)
                        {
                            std::string output = "Exception occured while accepting connection\n\t\t";
                            output += std::string(e.what());
                            aprint(output, 2);

                            try
                            {
                                close(new_socket);
                            }
                            catch (const std::exception &e)
                            {
                                aprint("Could not refuse connection. Its gone!", 2);
                            }

                            // goes to the next loop iteraction
                            continue;
                        }
                    }
                }
            }
            else if (result == 0)
            {
                // read timeout
                // no new connection was found
                // goes to the next iteration
            }
            else
            {
                aprint("Select reading error!", 2);
            }
        }
        aprint("Accept loop terminated.", 2);
    }
    catch (const std::exception &e)
    {
        std::string eoutput = "Error occured on server accept loop:\n\t\t" + std::string(e.what());
        aprint(eoutput, 2);
        raise(eoutput, 2);
    }
    catch (...)
    {
        raise("Unknown error occured on server accept loop!", 2);
    }
}

void ServerConnectionManager::add_servers()
{
    std::ifstream file ("./server-config");
    if (file.is_open()) 
    {
        std::string line;
        std::getline(file, line);
        int timeout = std::stoi(line.substr(line.find("=")+2, -1));
        sleep(timeout);

        std::getline(file, line);
        while(!file.eof())
        {
            std::getline (file, line);
            if(!line.empty())
            {
                std::string ip = strtok((char *) line.c_str(), " ");
                int port = std::stoi(strtok(NULL, " "));
                std::pair<std::string, int> address = std::make_pair(ip, port);

                int key = std::stoi(strtok(NULL, " "));
                bool coordinator = false;
                int socket = 0;

                if(strtok(NULL, " ") == NULL)
                {
                    Server_info server = { address, key, coordinator, socket };
                    this->server_backups_address_.emplace_back(server);
                }
                else
                {
                    this->set_address(ip);
                    this->set_port(port);
                    this->coordinator.store(coordinator);
                    this->key = key;
                }
            }
        }
    }
    else
    {
        aprint("Falha ao ler configs");
    }
}

int ServerConnectionManager::get_coordinator()
{
    for(auto &server : this->server_backups_address_)
    {
        if(server.coordinator)
        {
            return server.socket;
        }
    }
}