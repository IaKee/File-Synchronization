#ifndef CLIENT_COMMANDS_HPP
#define CLIENT_COMMANDS_HPP

#include <string>
#include <vector>
#include <functional>

void upload_command(const std::string& path);
void download_command(const std::string& path);
void delete_command(const std::string& path);
void list_server_command(const std::string& arg);
void list_client_command(const std::string& arg);
void get_sync_dir_command(const std::string& arg);
void exit_command(const std::string& arg);

extern std::vector<std::pair<std::string, std::function<void(const std::string&)>>> commands;

#endif
