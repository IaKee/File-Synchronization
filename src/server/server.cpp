// c++
#include <iostream>
#include <cstring>
#include <thread>
#include <exception>
#include <stdexcept>

// connection
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "../include/common/connection.hpp"

// locals
#include "client_connection.hpp"
#include "../include/common/json.hpp"
#include "../include/common/lang.hpp"
#include "../include/common/utils.hpp"
#include "../include/common/user_interface.hpp"
#include "server.hpp"

// namespace
using json = nlohmann::json;
using namespace server;

Server::Server()
	:	S_UI_(ui_mutex, ui_cv, ui_buffer, ui_sanitized_buffer),
		internet_manager()
{
	// init sequence
	std::string machine_name = get_machine_name();
	std::cout << "[STARTUP] Initializing at " << machine_name << "..." << std::endl;

	// settings init
	std::cout << "\t[SYNCWIZARD SERVER] Trying to restore settings from previous session..." << std::endl;

	if(!is_valid_path(config_file_path_))
	{
		// first initialization
		std::cout << "\t[SYNCWIZARD SERVER] Could not load previous settings, restoring defaults..." << std::endl;
		
		if(!is_valid_path(config_dir_))
		{
			// if no config dir exists - creates it
			if(!create_directory(config_dir_))
			{
				throw std::runtime_error("[SYNCWIZARD SERVER] Could not create settings folder! Please check system permissions!");
			}
			else
			{
				std::cout << "\t[SYNCWIZARD SERVER] Config directory created!" << std::endl;
			}
		}

		if(!create_file(config_file_path_))
		{
			throw std::runtime_error("[SYNCWIZARD SERVER] Could not create settings file! Please check system permissions!");
		}
		else
		{
			std::cout << "\t[SYNCWIZARD SERVER] Settings file created! Filling defaults..." << std::endl;
			
			// TODO: fill file with defaults here
			save_data_["default_port"] = 65534;

			save_json_to_file(save_data_, config_file_path_);
			std::cout << "\t[SYNCWIZARD SERVER] Defaults written to config file!" << std::endl;
		}

		if(!is_valid_path(sync_dir_))
		{
			if(!create_directory(sync_dir_))
			{
				throw std::runtime_error("[SYNCWIZARD SERVER] Could not create main sync_dir folder! Please check system permissions!");
			}
		}
	}
	
	try
	{
		// try to read settings file
		save_data_ = get_json_contents(config_file_path_);

		// fill attributes with the data retrieved from the save file
		default_port_ = save_data_["default_port"].get<int>();
	}
	catch(const std::exception& e)
	{
		std::cerr << "\t[SYNCWIZARD SERVER] Could not read settings file!" << std::endl;
		throw std::runtime_error(e.what());
	}
	
	internet_manager.create_socket();

	// set defaut port
	internet_manager.set_port(65534);

	internet_manager.create_server();

	std::string addr = internet_manager.get_address();
	int port = internet_manager.get_port();
	std::cout << "[STARTUP] Server running at " << addr <<
		":" << port << "(" << internet_manager.get_sock_fd() << ")" << std::endl;
	
	S_UI_.start();
};

Server::~Server()
{
	// destructor
	std::cout << "\t[SYNCWIZARD SERVER] Terminating server..." << std::endl;
	internet_manager.close_socket();
};

void Server::start()
{
	// starts accept thread to recieve new connection requests
	internet_manager.start_accepting_connections();
	accept_th_ = std::thread(
		[this]() 
		{
        	internet_manager.server_accept_loop();
    	});

	try
	{	
		running_ = true;
		// begins application main loop
		main_loop();
	}
	catch(const std::exception& e)
	{
		std::cerr << "[SYNCWIZARD SERVER] Error occured on start(): " << e.what() << std::endl;
		throw std::runtime_error(e.what());
	}
	catch(...)
	{
		std::cerr << "[SYNCWIZARD SERVER] Unknown error occured on start()!" << std::endl;
		throw std::runtime_error("[SYNCWIZARD SERVER] Unknown error occured on start()!");
	}
}

void Server::stop()
{
	// stop user interface and command processing
	try
	{
		std::cout << "\t[SYNCWIZARD SERVER] Closing, please wait..." << std::endl;

		stop_requested_.store(true);
		//internet_manager.running_accept_.store(false);
		accept_th_.join();
		
		S_UI_.stop();
		
		ui_cv.notify_all();

	}
	catch(const std::exception& e)
	{
		std::cerr << "\t[SYNCWIZARD SERVER] Error occured on stop(): " << e.what() << std::endl;
		throw std::runtime_error(e.what());
	}
	catch(...)
	{
		std::cerr << "\t[SYNCWIZARD SERVER] Unknown error occured on stop()!" << std::endl;
		throw std::runtime_error("Unknown error occured on stop()!");
	}
}

void Server::close()
{
	// closes remaining threads
	S_UI_.input_thread_.join();
	accept_th_.join();
}

/*
void Connection::handle_client(int client_socket)
{
	// processes connection requests
    try
    {
		// message size
        char buffer[1024] = {0};

		// sets identification timeout to 10 seconds
		// if the client does not respond within this period, the connection is terminated
		int timeout_in_seconds = 10;
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout_in_seconds), sizeof(timeout_in_seconds));

        // receive the username and hostname from the client
        int bytes_recieved = recv(client_socket, buffer, sizeof(buffer), 0);

        if(bytes_recieved > 0) 
		{
            std::string received_data(buffer, bytes_recieved);

            // Split the received data into username and hostname
            size_t delimiter_pos = received_data.find('|');
            if(delimiter_pos != std::string::npos) 
			{
                std::string username = received_data.substr(0, delimiter_pos);
                std::string hostname = received_data.substr(delimiter_pos + 1);

				client_connection::User* client_ptr = this->client_manager_.get_user(username);

				if(client_ptr == nullptr)
				{
					// checks if client had logged in before adds new client/session
				}
				else
				{
					// client is connected, checks if it is possible to add a new session
					int max_sessions = client_ptr->get_session_limit();
					int current_sessions = client_ptr->get_active_session_count();

					if(current_sessions < max_sessions)
					{
						// under the session limit, add client session as a active connection
					}
					else
					{
						// refuse client due to it expiring the session limit
					}
				}

				
                // Simulate validation of username and hostname
                if (is_valid_username_and_hostname(username, hostname)) 
				{
                    // Check if the maximum number of connections for this user has been reached (2 in this example)
                    if(count_connections_for_user(username) >= 2) 
					{
                        // Inform the client that the connection is refused due to too many connections
                        std::string refusal_reason = "Connection refused: Maximum number of connections reached for user '" + username + "'.";
                        send(client_socket, refusal_reason.c_str(), refusal_reason.size(), 0);
                        closesocket(client_socket); // Close the socket
                    } 
					else 
					{
                        // Add the client to the list of active connections for this user
                        add_client_to_user_connections(username, client_socket);

                        // Continue with the rest of the client handling logic here
                        // ...

                        // For demonstration purposes, we are closing the socket immediately after handling
                        closesocket(client_socket);
                    }
                } 
				else 
				{
                    // Inform the client that the connection is refused due to invalid username or hostname
                    std::string refusal_reason = "Connection refused: Invalid username or hostname.";
                    send(client_socket, refusal_reason.c_str(), refusal_reason.size(), 0);
                    closesocket(client_socket); // Close the socket
                }
            }
        }
		else if(bytes_recieved == 0)
		{
			std::cout << "Client closed the connection" << std::endl;
		}
		else
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK) 
			{
				std::cout << "Connection timeout" << std::endl;
			}
			else
			{
				std::cout << "Unknown connection error" << std::endl;
			}
		}
    }
    catch(const std::exception& e)
    {
        std::cerr << "[CONNECTION MANAGER] Error occurred on handle_client(): " << e.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "[CONNECTION MANAGER] Unknown error occurred on handle_client()!" << std::endl;
    }
}*/

/*void Server::process_connections(int client_socket) 
{
	// process a newly recieved connection request with a client
	char buffer[1024] = {0};
	int bytes_read;

	while((bytes_read = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
		//Agora, apos receber a mensagem, trata o tipo de mensagem recebida (1o caractere da string para cada metodo)
		// Handle the incoming message from the client
		std::cout << "Received message from client: " << buffer << std::endl;

		json jsonObject = json::parse(buffer);

		// Access the JSON data as needed
		std::string key1Value = jsonObject["function_type"];
		std::string username = jsonObject["username"];

		// Switch based on the first key
		if (key1Value == "uploadFile") {
			std::string fileContent = jsonObject["file_binary"];
			std::string filename = jsonObject["filename"];

			std::string filenamePath = sync_dir_ + "\\" + filename ;

			std::ofstream outFile(filenamePath, std::ios::binary);
			if (outFile) {
				outFile.write(fileContent.c_str(), fileContent.size());
				outFile.close();
			} else {
				std::cerr << "Error writing file" << std::endl;
			}
		} 
		else if (key1Value == "deleteFile") 
		{
			std::string filename = jsonObject["filename"];
			std::string filenamePath = sync_dir_ + "\\" + filename ;

			if (std::remove(filenamePath.c_str()) == 0) 
			{
			std::cout << "Arquivo " << filename << " excluído com sucesso." << std::endl;
			} 
			else 
			{
			std::cerr << "Erro ao excluir o arquivo " << filename << "." << std::endl;
			}
			
		} 
		else if (key1Value == "downloadFile") 
		{
			std::string filename = jsonObject["filename"];
			std::string filenamePath = sync_dir_ + "\\" + filename ;
			std::ifstream file(filenamePath, std::ios::binary);
			if (!file.is_open()) 
			{
				std::cerr << "Erro ao abrir o arquivo: " << filename << std::endl;
				return;
			}

			file.seekg(0, std::ios::end);
			int file_size = file.tellg();
			file.seekg(0, std::ios::beg);

			send(client_socket, &file_size, sizeof(file_size), 0);

			// Enviar o conteúdo do arquivo para o cliente
			char buffer[1024];
			while (!file.eof()) 
			{
				file.read(buffer, sizeof(buffer));
				send(clientSocket, buffer, file.gcount(), 0);
			}
			file.close();
							
		} 
		else 
		{
			std::cerr << "Received unknown key: " << firstKey << std::endl;
		}

		// Use the extracted values
		std::cout << "key1: " << key1Value << std::endl;
		std::cout << "key2: " << key2Value << std::endl;
		memset(buffer, 0, sizeof(buffer)); // Clear the buffer for the next message
	}

}*/

void Server::process_input()
{
	// process input buffer and calls methods accordingly
	int n_args = ui_sanitized_buffer.size();

	switch (n_args)
	{
		case 0:
			break;
		case 1:
			if(ui_sanitized_buffer.front() == "exit")
			{
				stop();
			}
			else
			{
				std::cout << "\t[SYNCWIZARD SERVER] No such command exists!" << std::endl;
				std::cout << "\t[SYNCWIZARD SERVER] Please enter 'help' for a detailed list commands for this application.!" << std::endl;
			}
			break;
		default:
			std::cout << "\t[SYNCWIZARD SERVER] No such command exists!" << std::endl;
			std::cout << "\t[SYNCWIZARD SERVER] Please enter 'help' for a detailed list commands for this application.!" << std::endl;
			break;
	}
}

void Server::main_loop()
{
	std::cout << "\t[SYNCWIZARD SERVER] Starting up server main loop..." << std::endl;
	try
	{
		while(running_)
		{  
			{
				// inserts prompt prefix before que actual user command
				std::lock_guard<std::mutex> lock(ui_mutex);
				std::cout << "\t#> " << std::flush;
			}   

			{
				std::unique_lock<std::mutex> lock(ui_mutex);
				ui_cv.wait(lock, [this]{return ui_buffer.size() > 0 || stop_requested_.load();});
			}
			//std::cout << "started loop running ui" << std::endl;

			// checks again if the client should be running
			if(stop_requested_.load())
			{
				ui_cv.notify_one();
				break;
			}
			
			process_input();

			// resets shared buffers
			ui_buffer.clear();
			ui_sanitized_buffer.clear();

			// notifies UI_ that the buffer is free to be used again
			ui_cv.notify_one();
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "\t[SYNCWIZARD SERVER] Error occured on main_loop(): " << e.what() << std::endl;
		throw std::runtime_error(e.what());
	}
	catch (...) 
	{
		std::cerr << "\t[SYNCWIZARD SERVER] Unknown error occured on main_loop()!" << std::endl;
		throw std::runtime_error("Unknown error occured on main_loop()");
	}
}