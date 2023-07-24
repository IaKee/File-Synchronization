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
			conn_.create_socket();
			conn_.set_port(65534);
			conn_.create_server();
			std::cout << "aguardando conexoes" << std::endl;

		};

		~Server()
		{
			// destrutor
			conn_.close_socket();
		};

		void main_loop()
		{
			
			while(true)
			{
				int client_socket = conn_.accept_connection();

				if(client_socket != -1)
				{
					// recebe uma mensagem
					char buffer[1024];
					int bytesRead = recv(client_socket, buffer, sizeof(buffer), 0);
					std::cout << "\tMensagem recebida do cliente: " << buffer << std::endl;
				}
				
				//conn_.handle_connection();
				sleep(1);
			}

				
			
		}
};


int main()
{
	Server server;

	server.main_loop();

}