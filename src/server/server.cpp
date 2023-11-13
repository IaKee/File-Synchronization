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
#include "../include/common/connection_manager.hpp"

// locals
#include "client_connection.hpp"
#include "../include/common/utils.hpp"
#include "../include/common/async_cout.hpp"
#include "../include/common/user_interface.hpp"
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

	// set defaut port
	int new_port = 65535;
	bool port_avaiable = false;

	// chooses a port that is avaiable by testing them
	while(port_avaiable == false && new_port > 50000)
	{
		new_port -= 1;
		port_avaiable = internet_manager.is_port_available(new_port);
	}
	internet_manager.set_port(new_port);

	internet_manager.create_server();

	std::string addr = internet_manager.get_address();
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

void Server::handle_new_session(int new_socket, std::string username, std::string machine)
{
	// processes new connection requests
    try
    {
		client_connection::User* new_user = client_manager_.get_user(username);
		if(new_user == nullptr)
		{
			// session is from a new user, add to the list
			aprint("Loading new session's user files...");
			client_manager_.load_user(username);

			// tries to retrieve newly added user
			aprint("Verifying loaded user user...");
			new_user = client_manager_.get_user(username);

			if(new_user == nullptr)
			{
				raise("User was wrongly or not added to the user manager!");
			}
		}
		
		aprint("Checking for existing sessions...");
		int active_sessions = new_user->get_active_session_count();
		int session_limit = 2;

		if(active_sessions < session_limit)
		{
			// checks if there is already a session on the given socket
			aprint("Checking for duplicate sessions...");
			client_connection::ClientSession* new_session = new_user->get_session(new_socket);

			if(new_session == nullptr)
			{	
				aprint("Creating a new session...");
				
				// before creating a new session, sends login confirmation to client
				std::string login_confirmation_command = "login|ok|" + std::to_string(active_sessions + 1);
				packet login_confirmation_packet;
				strcharray(
					login_confirmation_command, 
					login_confirmation_packet.command, 
					sizeof(login_confirmation_packet.command));
				
				// TODO: ensure safety here
				internet_manager.send_packet(login_confirmation_packet, new_socket);
				
				// creates new session instance
				// sends references to the general send/receive methods by reference
				std::unique_ptr<client_connection::ClientSession> created_session = 
					std::make_unique<client_connection::ClientSession>(
						new_socket,
						username,
						machine,
						sync_dir_,
						[this](const packet& p, int sockfd = -1, int timeout = -1) 
						{
							internet_manager.send_packet(p, sockfd, timeout);
						},
						[this](packet* p, int sockfd = -1, int timeout = -1) 
						{
							internet_manager.receive_packet(p, sockfd, timeout);
						},
						[new_user](int caller_sockfd, packet& p) 
						{
							new_user->broadcast_other_sessions(caller_sockfd, p);
						});

				// TODO: remove this
				std::string output = created_session->get_identifier();
				output += " session added! Force starting synchronization protocol.";
				// hack to force start the synchronization
            	packet sync_packet;
            	std::string command = "clist";
            	strcharray(command, sync_packet.command, sizeof(sync_packet.command));
				created_session->add_packet_from_broadcast(sync_packet);  // TODO: THIS IS WRONG!!!!
				
				// adds new session to the user manager
				new_user->add_session(created_session.release());
				
				output = created_session->get_identifier() + "logged in!";
			}
			else
			{
				raise("Session already logged in?");
			}
		}
		else
		{
			raise("User already has 2 sessions connected!");
		}
		
    }
    catch(const std::exception& e)
    {
		raise("Exception occurred while processing new session: \n\t\t" + std::string(e.what()));
    }
    catch(...)
    {
		raise("Unknown exception occurred while processing new session!");
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
				aprint("Could not find a command by \"" + ui_buffer + "\"!");
			}
			break;
		case 2:
			if(ui_sanitized_buffer.front() == "list")
			{
				if(ui_sanitized_buffer.back() == "clients")
				{
					std::list<std::string> connected = client_manager_.list_users();
					std::string output = "Currently these users are connected:";
					for(std::string s : connected)
					{
						output += "\n\t\t-> " + s;
					}
					aprint(output);
				}
				else
				{
					aprint("Could not find a command by \"" + ui_buffer + "\"!");
				}
			}
			else
			{
				aprint("Could not find a command by \"" + ui_buffer + "\"!");
			}
			break;
		default:
			aprint("Could not find a command by \"" + ui_buffer + "\"!");
			break;
	}
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
				//aprint("\t#> ", false);
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