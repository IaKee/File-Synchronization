// standard c++
#include <iostream>
#include <stdexcept>
#include <exception>
#include <csignal>

// standard c
#include <termios.h>
#include <unistd.h>

// locals
#include "server.hpp"
#include "../../common/include/external/cxxopts.hpp"
#include "../../common/include/utils.hpp"
#include "../../common/include/asyncio/async_cout.hpp"

using namespace async_cout;
using namespace server;

struct termios old_settings, new_settings;
void cleanup(int signal)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
	std::cout << "[CLEANUP] Restored terminal to cooked mode..." << std::endl;

	stop_capture();
	std::exit(signal);
}

int main()
{	
	// saves current terminal mode
	std::cout << "[MAIN] Saving terminal current mode..." << std::endl;
    tcgetattr(STDIN_FILENO, &old_settings);
	std::cout << "[MAIN] Setting terminal to raw mode..." << std::endl;
    new_settings = old_settings;
    new_settings.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
	std::signal(SIGINT, cleanup);
	
	// starts async cout
	start_capture();

	try
	{
		Server server;
		server.start();
	}
	catch(const std::exception& e)
    {
		std::string eoutput = "Exception captured:\n\t" + std::string(e.what());
        print(eoutput, "main", 0, 0, 0);
    }
	catch(...)
	{
		print("Unknwon exception captured!", "main", 0, 0, 0);
	}

	print("Running cleanup...", "main", 0, 0, 0);
	cleanup(0);
	return 0;
}