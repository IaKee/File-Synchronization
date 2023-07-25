#include "client_connection.hpp"

using namespace client_connection;

ClientConnection::ClientConnection(std::string user, int sock)
    :   username(user),
        sockfd(sock)
{
    
}