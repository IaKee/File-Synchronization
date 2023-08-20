#pragma once

// c++
#include <iostream>
#include <ctime>

// local
#include "json.hpp"

// namespace
using json = nlohmann::json;

bool is_valid_username(const std::string& username);
bool is_valid_address(const std::string& ip_address);
bool is_valid_port(int port);
bool is_valid_path(std::string path);
bool create_directory(const std::string& path);
bool create_file(const std::string& path);
json get_json_contents(const std::string& path);
void save_json_to_file(json data, const std::string& filename);
std::string get_machine_name();
std::time_t get_time();
int get_folder_space(std::string folder_path, std::string param);