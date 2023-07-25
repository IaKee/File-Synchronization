#pragma once

#include <string>
#include <thread>

namespace client_connection
{
    class ClientConnection
    {
        // client connection instance to server

        public:
            ClientConnection(std::string user, int sock);

            std::string username;
            int sockfd;
            
            std::thread main_th;
        private:
            //nothing here
    };
}