#pragma once

#include <iostream>
#include <regex>
#include <cstdlib>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <filesystem>

void insert_prefix();
bool is_valid_IP(const std::string& ip_address);
bool is_valid_port(int port);
bool is_valid_path(std::string path);
bool is_valid_username(const std::string& username);
