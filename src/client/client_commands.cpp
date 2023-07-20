#include <iostream>
#include <string>
#include "client_commands.hpp"
#include "../include/common/utils.hpp"

void upload_command(const std::string& path)
{
    insert_prefix();
    std::cout << "Upload command executed. Path: " << path << std::endl;
    insert_prefix();
    
    // Implementação para o comando de upload
}

void download_command(const std::string& path)
{
    insert_prefix();
    std::cout << "Download command executed. Path: " << path << std::endl;
    insert_prefix();
    // Implementação para o comando de download
}

void delete_command(const std::string& path)
{
    insert_prefix();
    std::cout << "Delete command executed. Path: " << path << std::endl;
    insert_prefix();
    // Implementação para o comando de delete
}

void list_server_command(const std::string& arg)
{
    insert_prefix();
    std::cout << "List Server command executed." << std::endl;
    insert_prefix();
    // Implementação para o comando de list_server
}

void list_client_command(const std::string& arg)
{
    insert_prefix();
    std::cout << "List Client command executed." << std::endl;
    insert_prefix();
    // Implementação para o comando de list_client
}

void get_sync_dir_command(const std::string& arg)
{
    insert_prefix();
    std::cout << "Get Sync Dir command executed." << std::endl;
    insert_prefix();
    // Implementação para o comando de get_sync_dir
}

void exit_command(const std::string& arg)
{
    insert_prefix();
    std::cout << "Exit command executed." << std::endl;
    insert_prefix();
    // Implementação para o comando de exit
}

void help_command(const std::string& arg)
{
    insert_prefix();
    std::cout << "Exit command executed." << std::endl;
    insert_prefix();
}

std::vector<std::pair<std::string, std::function<void(const std::string&)>>> command_list = 
{
    {"upload", upload_command},
    {"download", download_command},
    {"delete", delete_command},
    {"list_server", list_server_command},
    {"list_client", list_client_command},
    {"get_sync_dir", get_sync_dir_command},
    {"exit", exit_command},
    {"help", help_command}
};
