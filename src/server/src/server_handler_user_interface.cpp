#include "server.hpp"

using namespace server;

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