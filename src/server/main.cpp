#include <iostream>
#include <stdexcept>
#include <exception>

// locals
#include "server.hpp"

int main()
{
	try
	{
		server::Server server;
		server.start();
	}
	catch(const std::exception& e)
    {
        std::cerr << "Exception captured on main(): " << e.what() << std::endl;
    }
	catch(...)
	{
		 std::cerr << "Unknwon exception captured on main()!" << std::endl;
	}

}