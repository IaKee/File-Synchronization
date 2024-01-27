#include "server.hpp"

using namespace server;

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
					sizeof(login_confirmation_packet.command)+1);
				
				// safety here is questionable, however this should be single threaded
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
						},
						new_user);

				std::string output = created_session->get_identifier();
				output += " logged in!";
				
				// adds new session to the user manager
				new_user->add_session(created_session.release());
			}
			else
			{
				raise("Session already logged in? Socket busy.");
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