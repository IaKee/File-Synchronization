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
#include "../include/common/utils.hpp"
#include "../include/common/user_interface.hpp"
#include "server.hpp"

// namespace
using json = nlohmann::json;
using namespace server;

Server::Server()
	:	S_UI_(ui_mutex, ui_cv, ui_buffer, ui_sanitized_buffer),
		internet_manager(),
		stop_requested_(false)
{
	
	// init sequence
	std::string machine_name = get_machine_name();
	async_utils::async_print("[STARTUP] Initializing at " + machine_name + "...");

	// settings init
	async_utils::async_print("\t[SYNCWIZARD SERVER] Trying to restore settings from previous session...");

	if(!is_valid_path(config_file_path_))
	{
		// first initialization
		async_utils::async_print("\t[SYNCWIZARD SERVER] Could not load previous settings, restoring defaults...");
		
		if(!is_valid_path(config_dir_))
		{
			// if no config dir exists - creates it
			if(!create_directory(config_dir_))
			{
				throw std::runtime_error("[SYNCWIZARD SERVER] Could not create settings folder! Please check system permissions!");
			}
			else
			{
				async_utils::async_print("\t[SYNCWIZARD SERVER] Config directory created!");
			}
		}

		if(!create_file(config_file_path_))
		{
			throw std::runtime_error("[SYNCWIZARD SERVER] Could not create settings file! Please check system permissions!");
		}
		else
		{
			async_utils::async_print("\t[SYNCWIZARD SERVER] Settings file created! Filling defaults...");
			
			// TODO: fill file with defaults here
			save_data_["default_port"] = 65534;

			save_json_to_file(save_data_, config_file_path_);
			async_utils::async_print("\t[SYNCWIZARD SERVER] Defaults written to config file!");
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
		async_utils::async_print("\t[SYNCWIZARD SERVER] Could not read settings file!");
		throw std::runtime_error(e.what());
	}
	
	internet_manager.create_socket();

	// set defaut port
	internet_manager.set_port(65534);

	internet_manager.create_server();

	std::string addr = internet_manager.get_address();
	int port = internet_manager.get_port();
	async_utils::async_print("[STARTUP] Server running at " + addr + ":" + 
		std::to_string(port) + "(" + std::to_string(internet_manager.get_sock_fd()) + ")");
	
	S_UI_.start();
};

Server::~Server()
{
	close();
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
		// begins application main loop
		running_ = true;
		stop_requested_.store(false);
		main_loop();
	}
	catch(const std::exception& e)
	{
		async_utils::async_print("[SYNCWIZARD SERVER] Error occured on start(): " + std::string(e.what()));
		throw std::runtime_error(e.what());
	}
	catch(...)
	{
		async_utils::async_print("[SYNCWIZARD SERVER] Unknown error occured on start()!");
		throw std::runtime_error("[SYNCWIZARD SERVER] Unknown error occured on start()!");
	}
}

void Server::stop()
{
	// stop user interface and command processing
	try
	{
		async_utils::async_print("\t[SYNCWIZARD SERVER] Closing, please wait...");

		stop_requested_.store(true);
		running_ = false;
		
		S_UI_.stop();
		ui_cv.notify_all();

		internet_manager.stop_accepting_connections();
		accept_th_.join();
	}
	catch(const std::exception& e)
	{
		async_utils::async_print("\t[SYNCWIZARD SERVER] Error occured on stop(): " + std::string(e.what()));
		throw std::runtime_error(e.what());
	}
	catch(...)
	{
		async_utils::async_print("\t[SYNCWIZARD SERVER] Unknown error occured on stop()!");
		throw std::runtime_error("Unknown error occured on stop()!");
	}

	async_utils::async_print("\t[SYNCWIZARD SERVER] Main components closed!");
}

void Server::close()
{
	// closes remaining components/threads
	internet_manager.close_socket();
	S_UI_.input_thread_.join();
}

void Server::handle_new_session(int new_socket, std::string username, std::string machine)
{
	// processes new connection requests
    try
    {
		client_connection::User* new_user = client_manager_.get_user(username);
		if(new_user == nullptr)
		{
			// session is from a new user, add to the list
			client_manager_.load_user(username);

			// tries to retrieve newly added user
			new_user = client_manager_.get_user(username);

			if(new_user == nullptr)
			{
				throw std::runtime_error("[SYNCWIZARD SERVER] User was wrongly or not added to the user manager!");
			}
		}
		
		// checks if there is already a session on the given socket
		client_connection::ClientSession* new_session = new_user->get_session(new_socket);
		if(new_session == nullptr)
		{	
			// creates new session instance
			client_connection::ClientSession created_session(
				new_socket, 
				username, 
				machine,
				internet_manager.send_data,
				internet_manager.recieve_data,
				new_user->broadcast,
				new_user->remove_session)
			new_session = new_user->add_session(new_socket, created_session);

			// tries to retrieve newly added session
			client_connection::ClientSession* new_session = new_user->get_session(new_socket);
			if(new_session == nullptr)
			{
				throw std::runtime_error("[SYNCWIZARD SERVER] Session was wrongly or not added to the user manager!");
			}
		}
    }
    catch(const std::exception& e)
    {
        async_utils::async_print("\t[SYNCWIZARD SERVER] Exception occurred while processing new session: \n\t\t" + e.what());
    }
    catch(...)
    {
        async_utils::async_print("\t[SYNCWIZARD SERVER] Unknown exception occurred while processing new session!");
    }
}

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
				break;
			}
			else
			{
				async_utils::async_print("\t[SYNCWIZARD SERVER] Could not find a command by \"" + ui_buffer + "\"!");
			}
			break;
		default:
			async_utils::async_print("\t[SYNCWIZARD SERVER] Could not find a command by \"" + ui_buffer + "\"!");
			break;
	}
}


void Server::main_loop()
{
	async_utils::async_print("\t[SYNCWIZARD SERVER] Starting up server main loop...");
	try
	{
		while(running_)
		{  
			{
				// inserts prompt prefix before que actual user command
				std::lock_guard<std::mutex> lock(ui_mutex);
				//async_utils::async_print("\t#> ", false);
			}   

			{
				std::unique_lock<std::mutex> lock(ui_mutex);
				ui_cv.wait(lock, [this]{return ui_buffer.size() > 0 || stop_requested_.load();});
			}
			//std::cout << "started loop running ui" << std::endl;

			// checks again if the client should be running
			if(stop_requested_.load() == true)
			{
				ui_cv.notify_one();
				async_utils::async_print("\t[SYNCWIZARD SERVER] Main loop terminated!");
				return;
			}
			
			process_input();

			// resets shared buffers
			ui_buffer.clear();
			ui_sanitized_buffer.clear();

			// notifies UI_ that the buffer is free to be used again
			ui_cv.notify_one();
			
		}
		async_utils::async_print("\t[SYNCWIZARD SERVER] Main loop terminated!");
	}
	catch (const std::exception& e)
	{
		async_utils::async_print("\t[SYNCWIZARD SERVER] Error occured on main_loop(): ");
		throw std::runtime_error(e.what());
	}
	catch (...) 
	{
		async_utils::async_print("\t[SYNCWIZARD SERVER] Unknown error occured on main_loop()!");
		throw std::runtime_error("Unknown error occured on main_loop()");
	}
}