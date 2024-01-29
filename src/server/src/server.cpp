// c++
#include <iostream>
#include <cstring>
#include <thread>
#include <exception>
#include <stdexcept>
#include <queue>

// connection
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "../../common/include/network/connection_manager.hpp"

// locals
#include "./network/client_connection.hpp"
#include "./network/server_connection.hpp"
#include "../../common/include/utils.hpp"
#include "../../common/include/asyncio/async_cout.hpp"
#include "../../common/include/asyncio/user_interface.hpp"
#include "server.hpp"

// namespace
using namespace server;
using namespace async_cout;

Server::Server()
	:	S_UI_(
			&ui_mutex, 
			&ui_cv, 
			&ui_buffer, 
			&ui_sanitized_buffer),
		internet_manager(),
		stop_requested_(false),
		client_manager_(
			std::bind(&connection::ServerConnectionManager::send_packet, &internet_manager, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), 
			std::bind(&connection::ServerConnectionManager::receive_packet, &internet_manager, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
		server_manager_(
		std::bind(&connection::ServerConnectionManager::send_packet, &internet_manager, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), 
		std::bind(&connection::ServerConnectionManager::receive_packet, &internet_manager, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
{
	// init sequence
	std::string machine_name = get_machine_name();
	aprint("Initializing at " + machine_name + "...");

	if(!is_valid_path(sync_dir_))
	{
		if(!create_directory(sync_dir_))
		{
			throw std::runtime_error("Could not create sync_dir directory! Please check system permissions!");
		}
		else
		{
			aprint("Initializing server (root) files directory...");
		}
	}
	internet_manager.create_socket();
	internet_manager.add_servers();
	aprint(internet_manager.get_first_address().first + " " + std::to_string(internet_manager.get_first_address().second));
	internet_manager.open_server();

	// logica de conexao do cliente adaptada pro servidor FAE
	try
	{
			// initializing the variables to connect
			// gambiarra q eu fiz pro server2 entrar no server1, tem q ajeitar
			if (internet_manager.get_key() != 1)
			{
				std::string server_address = internet_manager.get_first_address().first;
				int server_port = internet_manager.get_first_address().second;
				// linha abaixo deve ser mudada, username temporario para teste
				std::string username = "backup";

				// creates socket
				aprint("Creating socket...");
				internet_manager.create_socket();

				// resolves host name
				aprint("Resolving host name...");
				std::string server_adjusted_address = internet_manager.get_host_by_name(server_address);

				// tries to connect to server
				aprint("Attempting connection to server...");
				internet_manager.connect_to_server(server_address, server_port);

				// after being connected tries to send login request
				aprint("Logging in as \'" + username + "\'...");
				int session_id = internet_manager.slogin(username, machine_name);
				aprint("Got following session id: " + std::to_string(session_id));

				// if(!is_valid_path(default_sync_dir_path_))
				// {
				//     if(!create_directory(default_sync_dir_path_))
				//     {
				//         throw std::runtime_error("Could not create sync_dir directory! Please check system permissions!");
				//     }
				//     else
				//     {
				//         aprint("Initializing client (root) files directory...");
				//     }
				// }

				// sets running flag to true
				// running_app_.store(true);

				// start_receiver();
				// start_sender();
				// start_inotify();

				// start_sync_();
			}
	}
	catch(const std::exception& e)
	{   
			// propagates error
			raise("Critical error intializing:\n\t\t" + std::string(e.what()));
	}
	// Fim da logica de conexao do cliente adaptada para o servidor

	std::string addr = internet_manager.get_host_by_name(machine_name);
	int port = internet_manager.get_port();
	aprint("Server running at " 
		+ addr 
		+ ":" 
		+ std::to_string(port) 
		+ "(" 
		+ std::to_string(internet_manager.get_sock_fd()) 
		+ ")");
	
	S_UI_.start();
};

Server::~Server()
{
	close();
};

void Server::start()
{
	running_ = true;
	stop_requested_.store(false);

	// starts accept thread to recieve new connection requests
	internet_manager.start_accept_loop();
	accept_th_ = std::thread(
		[this]() 
		{
        	internet_manager.server_accept_loop(
				[this](int new_socket, std::string username, std::string machine)
				{
					handle_new_session(new_socket, username, machine);
				},
				[this](int new_socket, std::string username, std::string machine)
				{
					handle_new_server(new_socket, username, machine);
				});
    	});

	try
	{	
		// begins application main loop
		main_loop();
	}
	catch(const std::exception& e)
	{
		raise("Error occured on start():\n\t" + std::string(e.what()));
	}
	catch(...)
	{
		raise("Unknown error occured on start()!");
	}
}

void Server::stop()
{
	// stop user interface and command processing
	try
	{
		aprint("Closing, please wait...");

		stop_requested_.store(true);
		running_.store(false);
		
		S_UI_.stop();
		ui_cv.notify_all();

		internet_manager.stop_accept_loop();
		accept_th_.join();
	}
	catch(const std::exception& e)
	{
		aprint("Error occured on stop(): " + std::string(e.what()));
		throw std::runtime_error(e.what());
	}
	catch(...)
	{
		aprint("Unknown error occured on stop()!");
		throw std::runtime_error("Unknown error occured on stop()!");
	}

	aprint("Main components closed!");
}

void Server::close()
{
	// closes remaining components/threads
	internet_manager.close_socket();
	S_UI_.input_thread_.join();
}

void Server::main_loop()
{
	aprint("Starting up server main loop...");
	try
	{
		while(running_)
		{  
			{
				// inserts prompt prefix before que actual user command
				std::lock_guard<std::mutex> lock(ui_mutex);
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
				aprint("Main loop terminated!");
				return;
			}
			
			process_input();

			// resets shared buffers
			ui_buffer.clear();
			ui_sanitized_buffer.clear();

			// notifies UI_ that the buffer is free to be used again
			ui_cv.notify_one();
			
		}
		aprint("Main loop terminated!");
	}
	catch (const std::exception& e)
	{
		raise("Error occured on main_loop(): " + std::string(e.what()));
	}
	catch (...) 
	{
		raise("Unknown error occured on main_loop()");
	}
}

void server::aprint(std::string content, bool endl)
{
	print(content, "syncwizard server", 1, -1, 1, endl);
}

void server::raise(std::string error)
{
	throw std::runtime_error("[SYNCWIZARD SERVER] " + error);
}