#pragma once

// c++
#include <iostream>
#include <ctime>
#include <mutex>
#include <list>
#include <condition_variable>

// multithread & synchronization 
#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>

// local
#include "json.hpp"

// namespace
using json = nlohmann::json;

// validation
bool is_valid_username(const std::string& username);
bool is_valid_address(const std::string& ip_address);
bool is_valid_port(int port);
bool is_valid_path(std::string path);

// file/directory manipulation
bool create_directory(const std::string& path);
bool create_file(const std::string& path);
void delete_file(const std::string& file_path);
json get_json_contents(const std::string& path);
void save_json_to_file(json data, const std::string& filename);
void rename_replacing(std::string& old_path, std::string& new_path);
std::string file_without_extension(std::string file_name); 
int get_folder_space(std::string folder_path, std::string param);
std::string get_file_name(std::string& path);
std::string calculate_md5_checksum(const std::string& file_path);
std::vector<std::vector<char>> bufferize_file(std::string file_path, std::size_t buffer_size);

// other utilities
const char* strchar(std::string& i);
void strcharray(std::string input_string, char* output_char_array, size_t max_size);
std::string charraystr(char* char_array, size_t size);
std::string get_machine_name();
std::time_t get_time();
std::vector<std::string> split_buffer(const char* buffer);