#pragma once

#include <iostream>

void insert_prefix();
bool is_valid_IP(const std::string& ip_address);
bool is_valid_port(int port);
bool is_valid_path(std::string path);
bool is_valid_username(const std::string& username);
std::string get_machine_name();