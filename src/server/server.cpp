// c++
#include <iostream>
#include <cstring>
#include <thread>

// connection
#include "../include/common/connection.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

class Server
{
	private:
		bool running_;

		connection::Connection conn_;

		std::thread accect_th_;
	public:

		Server()
			:	running_(true),
				conn_()
		{
			// init sequence
			std::cout << conn_.create_socket() << std::endl;
			conn_.set_port(65534);
			conn_.createServer();

		};

		~Server()
		{
			// destrutor
			conn_.close_socket();
		};

		void main_loop()
		{
			while(running_)
			{
				int client_socket = conn_.accept_connection();

				if(client_socket != -1)
				{
					// recebe uma mensagem
					char buffer[1024];
					int bytesRead = recv(client_socket, buffer, sizeof(buffer), 0);
					std::cout << "\tMensagem recebida do cliente: " << buffer << std::endl;
				}
				
				conn_.handle_connection();
				
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		}
};


int main()
{
	Server server;

	server.main_loop();

}