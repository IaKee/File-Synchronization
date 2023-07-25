// c++
#include <iostream>
#include <cstring>
#include <thread>

// connection
#include "../include/common/connection.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <nlohmann/json.hpp>

// locals
#include "../include/common/file_manager.hpp"
#include "../include/common/lang.hpp"
#include "../include/common/utils.hpp"
#include "../include/common/user_interface.hpp"

class Server
{
	private:
		// runtime control
		bool running_;
		std::atomic<bool> stop_requested_;

		// connection
		connection::Connection conn_;
		std::list<std::string> current_clients_;
		std::string client_default_path_;

		// I/O
		user_interface::UserInterface S_UI_;
		file_manager::FileManager f_manager_;
		std::string config_dir_ = "./config";
		std::string config_file_path_ = "./config/server.json";
		std::string sync_dir_ = "./sync_dir";

		// other
		void read_configs()
		{
			bool first_init_ = !is_valid_path(config_path_);

			if(first_init_)
			{
				// first initialization
				std::cout << PROMPT_PREFIX_SERVER << "running first initialization...";
				std::cout << PROMPT_PREFIX_SERVER << "attempting to create configuration file...";
				if(!f_manager_.create_directory(config_dir_))
				{
					throw std::runtime_error(ERROR_CREATING_FOLDER);
				}
				if(!f_manager_.create_empty_file(config_file_path_))
				{
					throw std::runtime_error(ERROR_CREATING_FILE);
				}

				std::cout << PROMPT_PREFIX_SERVER << "attempting to create home directory...";
				if(!f_manager_.create_directory(home_path_))
				{
					throw std::runtime_error(ERROR_CREATING_FOLDER);
				}
				std::cout << PROMPT_PREFIX_SERVER << "checking permissions...";
				if(!f_manager_.check_permissions(home_path_))
				{
					throw std::runtime_error(ERROR_CHECKING_PERMISSIONS);
				}
			}
			else
			{
				// not the first initialization, tries to read save file
				js::json json_data;
			}
		}
		
		std::thread accept_th_;
	public:

		Server()
			:	S_UI_(mutex_, cv_, command_buffer_, sanitized_commands_),
			f_manager_(),
			conn_()
		{
			// init sequence
			conn_.create_socket();
			conn_.set_port(65534);
			conn_.create_server();
			
			// TODO: 
			// TODO: load/create save file
			S_UI_.start();

			std::string machine_name = get_machine_name();
			std::cout << "[STARTUP] " << SERVER_PROGRAM_NAME << " " << SERVER_PROGRAM_VERSION <<
                " initialized at " << machine_name << std::endl;

			std::string addr = conn_.get_address();
			int port = conn_.get_port();
			std::cout << "[STARTUP] " << "Server running at" << addr << ":" << port << std::endl;

		};

		~Server()
		{
			// destrutor
			conn_.close_socket();
		};
		
		// synchronization
        std::mutex mutex_;
        std::condition_variable cv_;
        std::string command_buffer_;
        std::list<std::string> sanitized_commands_;

		/*
		//Função que vai deletar o arquivo no syncdir quando o cliente mandar uma mensagem de exclusao de arquivo
		void delete_file(const std::string& filename) 
		{
			std::string sync_dir = "sync_dir/"; // Diretório "sync_dir" no servidor

			std::string full_path = sync_dir + filename;
			if (std::remove(full_path.c_str()) == 0) {
				std::cout << "Arquivo " << filename << " excluído com sucesso." << std::endl;
			} else {
				std::cerr << "Erro ao excluir o arquivo " << filename << "." << std::endl;
			}
		}
		*/

		/*
		//aqui preciso ver como faço com o sockfd_
		void receive_file(int sockfd_, const std::string& sync_dir) 
		{
			// Receber o nome do arquivo do cliente
			char buffer[1024];
			int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
			if (bytes_received <= 0) {
				std::cerr << "Erro ao receber o nome do arquivo." << std::endl;
				return;
			}

			std::string filename(buffer, bytes_received);

			// Construir o caminho completo para o arquivo no diretório "sync_dir"
			std::string full_path = sync_dir + "/" + filename;

			// Criar o arquivo no diretório "sync_dir" para receber o conteúdo do cliente
			std::ofstream file(full_path, std::ios::binary);
			if (!file.is_open()) 
			{
				std::cerr << "Erro ao criar o arquivo " << full_path << "." << std::endl;
				return;
			}

			// Receber e salvar o conteúdo do arquivo do cliente
			bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
			if (bytes_received <= 0) 
			{
				break;
			}
			file.write(buffer, bytes_received);


			file.close();
			std::cout << "Arquivo " << filename << " recebido e salvo em " << full_path << std::endl;

			// Implementar a lógica para propagar o arquivo para todos os dispositivos do usuário conectados ao servidor.
		}
		*/

		void start()
		{
			try
            {
                main_loop();
            }
            catch(const std::exception& e)
            {
                std::cerr << "[ERROR][SERVER] Error occured on start(): " << e.what() << std::endl;
				throw std::runtime_error(e);
            }
            catch(...)
            {
                std::cerr << "[ERROR][SERVER] Unknown error occured on start()!" << std::endl;
				throw std::runtime_error("Unknown error occured on start()!");
			}
		}

		void stop()
        {
			// stop user interface and command processing
            try
            {
                std::cout << PROMPT_PREFIX_SERVER << EXIT_MESSAGE << std::endl;

                stop_requested_.store(true);
                running_ = false;
                
                S_UI_.stop();
                
                cv_.notify_all();
    
            }
            catch(const std::exception& e)
            {
                std::cerr << "[ERROR][CLIENT] Error occured on stop(): " << e.what() << std::endl;
                throw std::runtime_error(e);
            }
            catch(...)
            {
                std::cerr << "[ERROR][CLIENT] Unknown error occured on stop()!" << std::endl;
                throw std::runtime_error("Unknown error occured on stop()!");
            }
        }

		void close()
        {
			// closes remaining threads
            UI_.input_thread_.join();
        }
		
		void handleClient(int clientSocket) 
		{
			char buffer[1024] = {0};
			int bytesRead;

			while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
				//Agora, apos receber a mensagem, trata o tipo de mensagem recebida (1o caractere da string para cada metodo)
				// Handle the incoming message from the client
				std::cout << "Received message from client: " << buffer << std::endl;

				js jsonObject = js::parse(buffer);

				// Access the JSON data as needed
				std::string key1Value = jsonObject["function_type"];
				std::string username = jsonObject["username"];

				// Switch based on the first key
				if (key1Value == "uploadFile") {
					std::string fileContent = jsonObject["file_binary"];
					std::string filename = jsonObject["filename"];

					std::string filenamePath = home_path_ + "\\" + filename ;

					std::ofstream outFile(filenamePath, std::ios::binary);
					if (outFile) {
						outFile.write(fileContent.c_str(), fileContent.size());
						outFile.close();
					} else {
						std::cerr << "Error writing file" << std::endl;
					}
				} 
				else if (key1Value == "deleteFile") 
				{
					std::string filename = jsonObject["filename"];
					std::string filenamePath = home_path_ + "\\" + filename ;

					if (std::remove(filenamePath.c_str()) == 0) 
					{
					std::cout << "Arquivo " << filename << " excluído com sucesso." << std::endl;
					} 
					else 
					{
					std::cerr << "Erro ao excluir o arquivo " << filename << "." << std::endl;
					}
					
				} 
				else if (key1Value == "downloadFile") 
				{
					std::string filename = jsonObject["filename"];
					std::string filenamePath = home_path_ + "\\" + filename ;
					std::ifstream file(filenamePath, std::ios::binary);
					if (!file.is_open()) 
					{
						std::cerr << "Erro ao abrir o arquivo: " << filename << std::endl;
						return;
					}

					file.seekg(0, std::ios::end);
					int file_size = file.tellg();
					file.seekg(0, std::ios::beg);

					send(client_socket, &file_size, sizeof(file_size), 0);

					// Enviar o conteúdo do arquivo para o cliente
					char buffer[1024];
					while (!file.eof()) 
					{
						file.read(buffer, sizeof(buffer));
						send(clientSocket, buffer, file.gcount(), 0);
					}
					file.close();
									
				} 
				else 
				{
					std::cerr << "Received unknown key: " << firstKey << std::endl;
				}

				// Use the extracted values
				std::cout << "key1: " << key1Value << std::endl;
				std::cout << "key2: " << key2Value << std::endl;
			}

				memset(buffer, 0, sizeof(buffer)); // Clear the buffer for the next message
			}

		}

		void process_input()
        {
            // process input buffer and calls methods accordingly
            int nargs = sanitized_commands_.size();

            switch (nargs)
            {
                case 0:
                    break;
                case 1:
                    if(sanitized_commands_.front() == "exit")
                    {
                        stop();
                    }
                    else
                        std::cout << PROMPT_PREFIX_SERVER << ERROR_COMMAND_INVALID << std::endl;
                    break;
                default:
                    //std::cout << PROMPT_PREFIX << ERROR_COMMAND_USAGE << sanitized_commands_.front() << std::endl;
                    std::cout << PROMPT_PREFIX_SERVER << ERROR_COMMAND_INVALID << std::endl;
                    break;
            }
        }

		void main_loop()
		{
			try
			{
				while(running_)
				{  
					{
						// inserts promt prefix before que actual user command
						std::lock_guard<std::mutex> lock(mutex_);
						std::cout << PROMPT_PREFIX;
						std::cout.flush();
					}   

					{
						std::unique_lock<std::mutex> lock(mutex_);
						cv_.wait(lock, [this]{return command_buffer_.size() > 0 || stop_requested_.load();});
					}

					// checks again if the client should be running
					if(stop_requested_.load())
					{
						cv_.notify_one();
						break;
					}

					process_input();

					// resets shared buffers
					command_buffer_ = "";
					sanitized_commands_.clear();

					// notifies UI_ that the buffer is free to be used again
					cv_.notify_one();
				}
			}
			catch (const std::exception& e)
			{
				std::cerr << "[ERROR][SERVER] Error occured on main_loop(): " << e.what() << std::endl;
				throw std::runtime_error(e);
			}
			catch (...) 
			{
				std::cerr << "[ERROR][SERVER] Unknown error occured on main_loop()!" << std::endl;
				throw std::runtime_error("Unknown error occured on main_loop()");
			}

			
			int client_socket = conn_.accept_connection();

			if(client_socket != -1) //Aceitou um novo socket
			{
				clientThreads.emplace_back(handleClient, client_socket);
				clientThreads.back().detach();

			}

			sleep(1);
			
		}

};


int main()
{
	try
	{
		Server server;
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