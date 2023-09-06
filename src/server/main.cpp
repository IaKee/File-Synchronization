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
#include "../include/common/cxxopts.hpp"
#include "../include/common/utils.hpp"

struct termios old_settings, new_settings;
void cleanup(int signal)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
	std::cout << "\n[CLEANUP] Restored terminal to cooked mode..." << std::endl;

	async_utils::stop_capture();
	std::exit(signal);
}

int main()
{	
	// saves current terminal mode
	std::cout << "[MAIN][CLEANUP] Saving terminal current mode..." << std::endl;
    tcgetattr(STDIN_FILENO, &old_settings);
	std::cout << "[MAIN][CLEANUP] Setting terminal to raw mode..." << std::endl;
    new_settings = old_settings;
    new_settings.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
	std::signal(SIGINT, cleanup);
	async_utils::start_capture();

	try
	{
		server::Server server;
		server.start();
	}
	catch(const std::exception& e)
    {
        async_utils::async_print("[MAIN] Exception captured: " + std::string(e.what()));
    }
	catch(...)
	{
		async_utils::async_print("[MAIN] Unknwon exception captured!");
	}

	async_utils::async_print("\t[MAIN] running cleanup...");
	cleanup(0);
	return 0;
}