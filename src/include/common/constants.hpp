#pragma once

#include <string>

const std::string PROGRAM_NAME = "SyncWizard";
const std::string PROGRAM_VERSION = "1.0";
const std::string PROGRAM_DESCRIPTION = "SyncWizard is a file synchronization program. \
It allows you to synchronize your files on a remote server for access and sharing. \
All non-help command-line arguments are required to run. Below are the details \
about each one:";
const std::string USERNAME_DESCRIPTION = "Use this option to specify the username to which \
the files will be associated. This is important for identifying the synchronized files \
belonging to each user. It must be between 1 and 12 characters long, without spaces or \
symbols.";
const std::string SERVER_IP_DESCRIPTION = "Use this option to specify the IP address of the \
SyncWizard server you want to use. This will allow the program to connect to the correct \
server for file synchronization.";
const std::string SERVER_PORT_DESCRIPTION = "Use this option to specify the port number of \
the SyncWizard server you want to use. The program will use this port to establish a \
connection with the server.";
const std::string HELP_DESCRIPTION = "This option displays the description of the available \
program arguments. If you need help or have questions about how to use SyncWizard, you \
can use this option to get more information.";
const std::string EXAMPLE_USAGE = "Example usage: ./SyncWizard --username john \
--server_ip 192.168.0.100 --port 8080";
const std::string MISSING_COMMAND_OPTIONS = "Missing required command-line options!";
const std::string MAIN_USAGE = "Usage: ./SyncWizard -u <username> -s <server_ip_address> -p <port>";
const std::string ALTERNATIVE_USAGE = "Alternative usage: ./SyncWizard <username> <server_ip_address> <port>";
const std::string RUN_INFO = "Please run './SyncWizard -h' for more information.";
const std::string ERROR_PARSING = "Critical error parsing command-line options:";
const std::string USERNAME_ERROR = "Invalid username! It must be between 1 and 12 characters long, \
without spaces or symbols.";
const std::string IP_ERROR = "Invalid IP address! The correct format should be XXX.XXX.XXX.XXX, \
where X represents a number from 0 to 255, and there should be dots (.) separating the sections.";
const std::string PORT_ERROR = "Invalid port! The correct port should be between 0 and 65535." ;
const std::string INVALID_COMMAND = "No such command exists! Please enter 'help' for a more detailed \
list of available commands for this application.";
const std::string EXIT_MESSAGE = "Exiting the program...";