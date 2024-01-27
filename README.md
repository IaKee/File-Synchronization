# Description
The project aims to create a service that allows users to share and automatically synchronize files across multiple devices. The implementation is divided into two parts, with the first part focusing on topics such as threads, processes, socket communication, and process synchronization using mutexes and semaphores. Additional functionalities will be added in the second part.

The application is designed to run on Unix environments (Linux), although it can be developed on other platforms. It is implemented using the Transmission Control Protocol (TCP) socket API of Unix and C/C++ programming languages.

## Basic Features
The application consists of a server and a client component. The server is responsible for managing files for multiple remote users, while the client allows users to remotely access and synchronize their files stored on the server. The following basic features are supported:
- Multiple Users: The server should be capable of handling simultaneous requests from multiple users.
Multiple Sessions: A user can use the service simultaneously on up to two different devices.
- Consistent Storage Structures: Data storage structures on the server should be kept in a consistent state and protected from concurrent access.
- Synchronization: Whenever a user modifies a file in the 'sync_dir' directory on their device, the file should be updated on the server and synchronized with the 'sync_dir' directories of the user's other devices.
- Data Persistence on the Server: User directories and files should be restored when the server is restarted.
System Architecture

The project is divided into two parts, with the second part adding additional functionalities to the first part. It is recommended to implement a modular and extensible system with encapsulation of client and server communication functions into separate modules.

The suggested system architecture includes communication modules responsible for file sending and receiving operations. The file management module handles user directories, file data, and metadata storage. Each client is identified by their unique user identifier, and a separate directory is maintained for each client.

Synchronization is linked to the 'sync_dir' directory on the client, similar to how Dropbox functions, where only files within the application's designated folder are synchronized with the server. The synchronization module on the client periodically checks the state of files, ensuring that the server and other devices remain updated according to the user's most recent modifications. For example, if a file is removed from the 'sync_dir' on one device, this change should be detected by the server and applied to the other active devices of the same user.

## User Interface
The client can establish a session with the server via the command line using the following format:

```php
># ./myClient <username> <server_ip_address> <port>
Where <username> represents the user identifier, and <server_ip_address> and <port> represent the server's IP address and port, respectively.
```

Once a session is initiated, the user can drag and drop files into the 'sync_dir' directory using the operating system's file manager. These files will be automatically synchronized with the server and the user's other devices. Similarly, the user can edit or delete files, and these modifications will be reflected automatically on the server and other devices.

In addition, a command-line interface should be accessible, allowing the user to perform basic system operations as detailed in the following table:

### Command	Description
```
# upload <path/filename.ext>	Uploads the file filename.ext to the server, placing it in the server's 'sync_dir' and propagating it to all the user's devices. Example: upload /home/user/MyFolder/filename.ext
# download <filename.ext>	Downloads the file filename.ext from the server to the client's 'sync_dir'. Example: download filename.ext
# delete <filename.ext>	Deletes the file filename.ext from the server and propagates the deletion to all the user's devices. Example: delete filename.ext
# list	Lists all files stored in the server's 'sync_dir'. Example: list
# quit	Ends the current session with the server. Example: quit
```

```
File-Synchronization
├─ .vscode
│  ├─ c_cpp_properties.json
│  ├─ launch.json
│  ├─ settings.json
│  └─ tasks.json
├─ Makefile
├─ README.md
├─ client
│  ├─ obj
│  ├─ src
│  │  ├─ application.cpp
│  │  ├─ application.hpp
│  │  ├─ inotify
│  │  │  └─ handler.cpp
│  │  ├─ main.cpp
│  │  ├─ network
│  │  │  ├─ client_network_handler.cpp
│  │  │  ├─ network_commands_client.cpp
│  │  │  └─ network_commands_server.cpp
│  │  └─ ui
│  │     └─ client_ui_handler.cpp
│  └─ tests
├─ common
│  └─ include
│     ├─ async_cout.cpp
│     ├─ async_cout.hpp
│     ├─ connection
│     │  ├─ connection_manager.cpp
│     │  ├─ connection_manager.hpp
│     │  ├─ connection_manager_client.cpp
│     │  └─ connection_manager_server.cpp
│     ├─ cxxopts.hpp
│     ├─ inotify_watcher.cpp
│     ├─ inotify_watcher.hpp
│     ├─ json.hpp
│     ├─ lang.hpp
│     ├─ user_interface.cpp
│     ├─ user_interface.hpp
│     ├─ utils.cpp
│     ├─ utils.hpp
│     ├─ utils_packet.cpp
│     └─ utils_packet.hpp
├─ server
│  ├─ obj
│  ├─ src
│  │  ├─ client_connection.cpp
│  │  ├─ client_connection.hpp
│  │  ├─ main.cpp
│  │  ├─ network
│  │  │  ├─ client_connection_session.cpp
│  │  │  ├─ client_connection_session_commands.cpp
│  │  │  ├─ client_connection_session_network.cpp
│  │  │  ├─ client_connection_user.cpp
│  │  │  ├─ managers
│  │  │  │  └─ client_connection_group.cpp
│  │  │  └─ methods
│  │  ├─ server
│  │  ├─ server.cpp
│  │  ├─ server.hpp
│  │  ├─ server_handler_new_sessions.cpp
│  │  └─ server_handler_user_interface.cpp
│  └─ tests
│     └─ ex.cpp
└─ sync_dir_server
   └─ iakee

```