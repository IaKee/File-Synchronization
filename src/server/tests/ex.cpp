#include <iostream>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

class Server {
private:
    int server_socket_;
    int signal_pipe_[2]; // Pipe for signaling

public:
    Server(int port) {
        server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        // Configurar o socket, bind, listen, etc.
        pipe(signal_pipe_); // Create the signal pipe
    }

    void start() {
        std::thread accept_thread(&Server::acceptConnections, this);

        while (true) {
            // Faz alguma operação aqui...

            // Mais operações aqui...
        }

        accept_thread.join();
    }

    void stop() {
        // Close the write end of the signal pipe to signal the accepting thread to stop
        close(signal_pipe_[1]);
    }

private:
    void acceptConnections() {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_socket_, &read_fds);
        FD_SET(signal_pipe_[0], &read_fds);

        int max_fd = std::max(server_socket_, signal_pipe_[0]);

        while (true) {
            fd_set temp_fds = read_fds;

            int result = select(max_fd + 1, &temp_fds, nullptr, nullptr, nullptr);
            if (result > 0) {
                if (FD_ISSET(server_socket_, &temp_fds)) {
                    // Aguarda a chegada de uma nova conexão
                    int new_socket = accept(server_socket_, nullptr, nullptr);
                    if (new_socket >= 0) {
                        // Nova conexão aceita com sucesso!
                        std::cout << "Nova conexão aceita!" << std::endl;

                        // Trate a nova conexão aqui...

                        close(new_socket); // Feche a conexão após o tratamento (se necessário)
                    }
                }

                if (FD_ISSET(signal_pipe_[0], &temp_fds)) {
                    // Signal received, time to stop accepting connections
                    std::cout << "Parando de aceitar novas conexões." << std::endl;
                    break;
                }
            }
        }

        close(signal_pipe_[0]); // Close the read end of the signal pipe
    }
};

int main() {
    Server server(8080);
    server.start();
    return 0;
}