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
#include "../include/common/file_manager.hpp"
#include "../include/common/json.hpp"
#include "../include/common/lang.hpp"
#include "../include/common/utils.hpp"
#include "../include/common/user_interface.hpp"
#include "server.hpp"

// namespace
using json = nlohmann::json;
using namespace server;

Server::Server()
	:	S_UI_(mutex_, cv_, command_buffer_, sanitized_commands_),
	f_manager_(),
	conn_(),
	running_(true),
	stop_requested_(false)
{
	// init sequence
	std::string machine_name = get_machine_name();
	std::cout << "[STARTUP] " << SERVER_PROGRAM_NAME << " " << SERVER_PROGRAM_VERSION <<
		" initializing at " << machine_name << std::endl;

	std::cout << PROMPT_PREFIX_SERVER << SOCK_CREATING;
	conn_.create_socket();
	std::cout << DONE_SUFIX << std::endl;


	// set defaut port
	conn_.set_port(65534);

	std::cout << PROMPT_PREFIX_SERVER << BINDING_PORT;
	conn_.create_server();
	std::cout << DONE_SUFIX << std::endl;
	
	// TODO: 
	// TODO: load/create save file

	std::cout << PROMPT_PREFIX_SERVER << UI_STARTING;
	S_UI_.start();
	std::cout << DONE_SUFIX << std::endl;

	std::string addr = conn_.get_address();
	int port = conn_.get_port();
	std::cout << "[STARTUP] " << "Done! Server running at " << addr << ":" << port << std::endl;

};

Server::~Server()
{
	// destructor
	conn_.close_socket();
};

void Server::start()
{
	// starts accept thread to recieve new connection requests
	conn_.link_pipe(signal_pipe_);
	accept_th_ = std::thread(
		[this]() 
		{
        	conn_.start_accepting_connections();
    	});

	try
	{	
		// TODO: start routine comms with clients

		// beggins application main loop
		main_loop();
	}
	catch(const std::exception& e)
	{
		std::cerr << "[ERROR][SERVER] Error occured on start(): " << e.what() << std::endl;
		throw std::runtime_error(e.what());
	}
	catch(...)
	{
		std::cerr << "[ERROR][SERVER] Unknown error occured on start()!" << std::endl;
		throw std::runtime_error("Unknown error occured on start()!");
	}
}

void Server::read_configs_()
{
	bool first_init_ = !is_valid_path(config_dir_);

	if(first_init_)
	{
		// first initialization
		std::cout << PROMPT_PREFIX_SERVER << "running first initialization...";
		std::cout << PROMPT_PREFIX_SERVER << "attempting to create configuration file...";
		if(!f_manager_.create_directory(config_dir_))
		{
			throw std::runtime_error(ERROR_CREATING_FOLDER);
		}
		if(!f_manager_.create_empty_file(config_file_path_))
		{
			throw std::runtime_error(ERROR_CREATING_FILE);
		}

		std::cout << PROMPT_PREFIX_SERVER << "attempting to create home directory...";
		if(!f_manager_.create_directory(sync_dir_))
		{
			throw std::runtime_error(ERROR_CREATING_FOLDER);
		}
		std::cout << PROMPT_PREFIX_SERVER << "checking permissions...";
		if(!f_manager_.check_permissions(sync_dir_))
		{
			throw std::runtime_error(ERROR_CHECKING_PERMISSIONS);
		}
	}
	else
	{
		// not the first initialization, tries to read save file
		json json_data = f_manager_.get_json_contents(config_file_path_);

		// TODO: restore stuff here
	}
}

void Server::stop()
{
	// stop user interface and command processing
	try
	{
		std::cout << PROMPT_PREFIX_SERVER << EXIT_MESSAGE << std::endl;

		stop_requested_.store(true);
		conn_.running_accept_.store(false);
		accept_th_.join();
		
		S_UI_.stop();
		
		cv_.notify_all();

	}
	catch(const std::exception& e)
	{
		std::cerr << "[ERROR][SERVER] Error occured on stop(): " << e.what() << std::endl;
		throw std::runtime_error(e.what());
	}
	catch(...)
	{
		std::cerr << "[ERROR][SERVER] Unknown error occured on stop()!" << std::endl;
		throw std::runtime_error("Unknown error occured on stop()!");
	}
}

void Server::close()
{
	// closes remaining threads
	S_UI_.input_thread_.join();
	accept_th_.join();
}

/*void Server::handleClient(int clientSocket) 
{
	char buffer[1024] = {0};
	int bytesRead;

	while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
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
	int nargs = sanitized_commands_.size();

	switch (nargs)
	{
		case 0:
			break;
		case 1:
			if(sanitized_commands_.front() == "exit")
			{
				stop();
			}
			else
			{
				std::cout << PROMPT_PREFIX_SERVER << ERROR_COMMAND_INVALID << std::endl;
			}
			break;
		default:
			//std::cout << PROMPT_PREFIX << ERROR_COMMAND_USAGE << sanitized_commands_.front() << std::endl;
			std::cout << PROMPT_PREFIX_SERVER << ERROR_COMMAND_INVALID << std::endl;
			break;
	}
}

void Server::main_loop()
{
	try
	{
		while(running_)
		{  
			{
				// inserts prompt prefix before que actual user command
				std::lock_guard<std::mutex> lock(mutex_);
				std::cout << PROMPT_PREFIX;
				std::cout.flush();
			}   

			{
				std::unique_lock<std::mutex> lock(mutex_);
				cv_.wait(lock, [this]{return command_buffer_.size() > 0 || stop_requested_.load();});
			}
			//std::cout << "started loop running ui" << std::endl;

			// checks again if the client should be running
			if(stop_requested_.load())
			{
				cv_.notify_one();
				break;
			}
			

			process_input();

			// resets shared buffers
			command_buffer_ = "";
			sanitized_commands_.clear();

			// notifies UI_ that the buffer is free to be used again
			cv_.notify_one();
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "[ERROR][SERVER] Error occured on main_loop(): " << e.what() << std::endl;
		throw std::runtime_error(e.what());
	}
	catch (...) 
	{
		std::cerr << "[ERROR][SERVER] Unknown error occured on main_loop()!" << std::endl;
		throw std::runtime_error("Unknown error occured on main_loop()");
	}

	std::cout << "retornou" << std::endl;
}