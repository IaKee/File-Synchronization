#pragma once

#include <iostream>
#include <string>


// parameter descriptions
const std::string SERVER_PROGRAM_NAME = "SyncWizard server";
const std::string SERVER_PROGRAM_VERSION = "1.0";
const std::string SERVER_IP_DESCRIPTION = "Use this option to specify the IP address of the \
SyncWizard server you want to use. This will allow the program to connect to the correct \
server for file synchronization.";
const std::string SERVER_PORT_DESCRIPTION = "Use this option to specify the port number of \
the SyncWizard server you want to use. The program will use this port to establish a \
connection with the server.";
const std::string HELP_DESCRIPTION = "This option displays the description of the available \
program arguments. If you need help or have questions about how to use SyncWizard, you \
can use this option to get more information.";
const std::string RUN_INFO = "Please run './SyncWizard -h' for more information.";
const std::string USERNAME_DESCRIPTION = "Use this option to specify the username to which \
the files will be associated. This is important for identifying the synchronized files \
belonging to each user. It must be between 1 and 12 characters long, without spaces or \
symbols.";

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
