#pragma once

#include <iostream>
#include <string>

const std::string CLIENT_PROGRAM_NAME = "SyncWizard client";
const std::string CLIENT_PROGRAM_VERSION = "1.0";
const std::string CLIENT_PROGRAM_DESCRIPTION = "SyncWizard client is a file synchronization program. \
It allows you to synchronize your files on a remote server for access and sharing. \
All non-help command-line arguments are required to run. Below are the details \
about each one:";
const std::string CLIENT_USAGE_LAYOUT = "Usage: ./SWizClient -u <username> -s <server_ip_address> -p <port>";
const std::string CLIENT_ALT_USAGE_LAYOUT = "Alternative usage: ./SWizClient <username> <server_ip_address> <port>";
const std::string CLIENT_EXAMPLE_USAGE = "Example usage: ./SWizClient --username john \
--server_ip 192.168.0.100 --port 8080";

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
const std::string ERROR_PARSING_CRITICAL = "Critical error parsing command-line options:";
const std::string ERROR_PARSING_MISSING = "Missing required command-line options!";
const std::string ERROR_PARSING_USERNAME = "Invalid username! It must be between 1 and 12 characters long, \
without spaces or symbols.";
const std::string ERROR_PARSING_IP = "Invalid IP address! The correct format should be XXX.XXX.XXX.XXX, \
where X represents a number from 0 to 255, and there should be dots (.) separating the sections.";
const std::string ERROR_PARSING_PORT = "Invalid port! The correct port should be between 0 and 65535." ;
const std::string ERROR_COMMAND_INVALID = "No such command exists! Please enter 'help' for a more detailed \
list of available commands for this application.";
const std::string ERROR_COMMAND_USAGE = "Incorrect usage of command: ";
const std::string ERROR_RESOLVING_HOST = "Error resolving host name.";
const std::string ERROR_CONNECTING_SERVER = "Error connecting to server.";
const std::string ERROR_CREATING_SERVER = "Error creating server!";
const std::string ERROR_BIND = "Error binding port to socket!";
const std::string ERROR_LISTEN = "Error setting socket to passive using listen";
const std::string ERROR_CONVERTING_IP = "Error converting IP address to string.";
const std::string ERROR_CREATING_FOLDER = "Could not create directory! Please check system permissions!";
const std::string ERROR_CREATING_FILE = "Could not create file! Please check system permissions!";
const std::string ERROR_READING_FILE = "Could not read from file! Please check system permissions!";
const std::string ERROR_CHECKING_PERMISSIONS = "Could not acess directory! Please check system permissions!";
const std::string ERROR_SOCK_CREATING = "Error creating socket!";
const std::string ERROR_PATH_INVALID = "Could not acess given path!";
const std::string ERROR_READING_CRITICAL = "Critical error reading command line!";

const std::string UI_STARTING = "Starting user interface";
const std::string SOCK_CREATING = "Creating socket...";
const std::string DONE_SUFIX = " --> Done!";
const std::string RESOLVING_HOST = "Resolving host...";
const std::string CONNECTING_TO_SERVER = "Attempting connection to server...";
const std::string BINDING_PORT = "Binding port to listen...";
const std::string EXIT_MESSAGE = "Exiting the program...";

// other
const std::string PROMPT_PREFIX = "\t#> ";
const std::string PROMPT_PREFIX_CLIENT = "\t[CLIENT] ";
const std::string PROMPT_PREFIX_SERVER = "\t[SERVER] ";
const std::string PROMPT_CLS = "\033[2J";
