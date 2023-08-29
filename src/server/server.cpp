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
		stop_requested_(false),
		client_manager_(
			std::bind(&connection::Connection::send_data, &internet_manager, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4), 
			std::bind(&connection::Connection::recieve_data, &internet_manager, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4))
{
	
	// init sequence
	std::string machine_name = get_machine_name();
	async_utils::async_print("\t[SYNCWIZARD SERVER] Initializing at " + machine_name + "...");

	if(!is_valid_path(sync_dir_))
	{
		if(!create_directory(sync_dir_))
		{
			throw std::runtime_error("\t[SYNCWIZARD SERVER] Could not create sync_dir directory! Please check system permissions!");
		}
		else
		{
			async_utils::async_print("\t[SYNCWIZARD SERVER] Initializing server (root) files directory...");
		}
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
        	internet_manager.server_accept_loop(
				[this](int new_socket, std::string username, std::string machine)
				{
					handle_new_session(new_socket, username, machine);
				});
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
		throw std::runtime_error("[SYNCWIZARD SERVER] Error occured on start(): " + std::string(e.what()));
	}
	catch(...)
	{
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
			async_utils::async_print("\t[SYNCWIZARD SERVER] loading user...");
			client_manager_.load_user(username);

			// tries to retrieve newly added user
			async_utils::async_print("\t[SYNCWIZARD SERVER] checking loaded user...");
			new_user = client_manager_.get_user(username);

			if(new_user == nullptr)
			{
				throw std::runtime_error("[SYNCWIZARD SERVER] User was wrongly or not added to the user manager!");
			}
		}
		
		// checks if there is already a session on the given socket
		async_utils::async_print("\t[SYNCWIZARD SERVER] checking if session exists...");
		client_connection::ClientSession* new_session = new_user->get_session(new_socket);

		if(new_session == nullptr)
		{	
			async_utils::async_print("\t[SYNCWIZARD SERVER] creating new session...");
			
			// creates new session instance
			std::unique_ptr<client_connection::ClientSession> created_session = 
				std::make_unique<client_connection::ClientSession>(
					new_socket,
					username,
					machine,
					sync_dir_,
					[this](char* buffer, std::size_t buffer_size, int socket_id, int flags) 
					{
						internet_manager.send_data(buffer, buffer_size, socket_id, flags);
					},
					[this](char* buffer, std::size_t buffer_size, int socket_id, int flags) 
					{
						internet_manager.recieve_data(buffer, buffer_size, socket_id, flags);
					},
					[new_user](char* buffer, std::size_t buffer_size) 
					{
						new_user->broadcast(buffer, buffer_size);
					});
			async_utils::async_print("\t[SYNCWIZARD SERVER] session created!");

			// adds new session to the user manager
			new_user->add_session(created_session.release());
			async_utils::async_print("\t[SYNCWIZARD SERVER] " + username + "(" + std::to_string(new_socket) + ") logged in!");
		}
		else
		{
			throw std::runtime_error("\t[SYNCWIZARD SERVER] Session already logged in?");
		}
    }
    catch(const std::exception& e)
    {
		throw std::runtime_error("[SYNCWIZARD SERVER] Exception occurred while processing new session: \n\t\t" + std::string(e.what()));
    }
    catch(...)
    {
		throw std::runtime_error("[SYNCWIZARD SERVER] Unknown exception occurred while processing new session!");
    }
}

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
		case 2:
			if(ui_sanitized_buffer.front() == "list")
			{
				if(ui_sanitized_buffer.back() == "clients")
				{
					std::list<std::string> connected = client_manager_.list_users();
					std::string output = "\t[SYNCWIZARD SERVER] Currently these users are connected:";
					for(std::string s : connected)
					{
						output += "\n\t\t-> " + s;
					}
					async_utils::async_print(output);
				}
				else
				{
					async_utils::async_print("\t[SYNCWIZARD SERVER] Could not find a command by \"" + ui_buffer + "\"!");
				}
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