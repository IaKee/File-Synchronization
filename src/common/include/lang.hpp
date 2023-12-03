#pragma once

#include <iostream>
#include <string>

// TODO: this file should not exist - there is no need

// parameter descriptions
const std::string SERVER_PROGRAM_NAME = "SyncWizard server";
const std::string SERVER_PROGRAM_VERSION = "1.0";

// error messages
const std::string ERROR_CONNECTING_SERVER = "Error connecting to server.";
const std::string ERROR_CREATING_SERVER = "Error creating server!";
const std::string ERROR_LISTEN = "Error setting socket to passive using listen";
const std::string ERROR_CREATING_FILE = "Could not create file! Please check system permissions!";
const std::string ERROR_READING_FILE = "Could not read from file! Please check system permissions!";
const std::string ERROR_CHECKING_PERMISSIONS = "Could not acess directory! Please check system permissions!";
const std::string ERROR_SOCK_CREATING = "Error creating socket!";
const std::string ERROR_GETTING_USER = "Could not find user logged in!";
const std::string CONNECTING_TO_SERVER = "Attempting connection to server...";
const std::string BINDING_PORT = "Binding port to listen...";
const std::string EXIT_MESSAGE = "Exiting the program...";
