
MAKEFLAGS += -j5

OBJS	= main.o server_handler_user_interface.o client_connection_session.o client_connection_user.o client_connection_group.o client_connection.o client_connection_session_commands.o client_connection_session_network.o server.o server_handler_new_sessions.o 
OUT	= server,client,common

OBJS0	= smain.o server_handler_user_interface.o client_connection_session.o client_connection_user.o client_connection_group.o client_connection.o client_connection_session_commands.o client_connection_session_network.o server.o server_handler_new_sessions.o
SOURCE0	= ./src/server/src/main.cpp ./src/server/src/server_handler_user_interface.cpp ./src/server/src/network/managers/client_connection_session.cpp ./src/server/src/network/managers/client_connection_user.cpp ./src/server/src/network/managers/client_connection_group.cpp ./src/server/src/network/client_connection.cpp ./src/server/src/network/commands/client_connection_session_commands.cpp ./src/server/src/network/commands/client_connection_session_network.cpp ./src/server/src/server.cpp ./src/server/src/server_handler_new_sessions.cpp ./src/server/tests/ex.cpp
HEADER0	= 
DEP0 	= 
OUT0	= server

OBJS1	= application.o cmain.o client_network_handler.o network_commands_client.o network_commands_server.o client_ui_handler.o handler.o 
SOURCE1	= ./src/client/src/application.cpp ./src/client/src/main.cpp ./src/client/src/network/client_network_handler.cpp ./src/client/src/network/network_commands_client.cpp ./src/client/src/network/network_commands_server.cpp ./src/client/src/ui/client_ui_handler.cpp ./src/client/src/inotify/handler.cpp
HEADER1	= 
DEP1 	= 
OUT1	= client

OBJS2	= inotify_watcher.o async_cout.o user_interface.o logging.o connection_manager.o server_commands.o socket_commands.o client_commands.o packet.o utils.o 
SOURCE2	= ./src/common/include/inotify_watcher.cpp ./src/common/include/asyncio/async_cout.cpp ./src/common/include/asyncio/user_interface.cpp ./src/common/include/network/logging.cpp ./src/common/include/network/connection_manager.cpp ./src/common/include/network/server_commands.cpp ./src/common/include/network/socket_commands.cpp ./src/common/include/network/client_commands.cpp ./src/common/include/network/packet.cpp ./src/common/include/utils.cpp
HEADER2	= 
DEP2 	=
OUT2	= common.o

CC	 = g++
FLAGS	 = -g -c -Wall
LFLAGS	 = -lpthread

all: server client 

runclient: client
	./client

runserver: server
	./server

test: all
	tmux new-session -s "server" -d "./server; read"
	tmux split-window -h "sleep 2 && ./client iakee 0.0.0.0 65535; read"
	tmux -2 attach-session -d

server: $(OBJS0) $(LFLAGS) $(DEP0) $(OBJS2)
	$(CC) -g $(OBJS0) $(OBJS2) -o $(OUT0) $(LFLAGS)

client: $(OBJS1) $(LFLAGS) $(DEP1) $(OBJS2)
	$(CC) -g $(OBJS1) $(OBJS2) -o $(OUT1) $(LFLAGS)

# common: $(OBJS2) $(LFLAGS) $(DEP2)
# 	$(CC) -g $(OBJS2) -o $(OUT2) $(LFLAGS)

smain.o: ./src/server/src/main.cpp
	$(CC) $(FLAGS) ./src/server/src/main.cpp -o smain.o

server_handler_user_interface.o: ./src/server/src/server_handler_user_interface.cpp
	$(CC) $(FLAGS) ./src/server/src/server_handler_user_interface.cpp 

client_connection_session.o: ./src/server/src/network/managers/client_connection_session.cpp
	$(CC) $(FLAGS) ./src/server/src/network/managers/client_connection_session.cpp 

client_connection_user.o: ./src/server/src/network/managers/client_connection_user.cpp
	$(CC) $(FLAGS) ./src/server/src/network/managers/client_connection_user.cpp 

client_connection_group.o: ./src/server/src/network/managers/client_connection_group.cpp
	$(CC) $(FLAGS) ./src/server/src/network/managers/client_connection_group.cpp 

client_connection.o: ./src/server/src/network/client_connection.cpp
	$(CC) $(FLAGS) ./src/server/src/network/client_connection.cpp 

client_connection_session_commands.o: ./src/server/src/network/commands/client_connection_session_commands.cpp
	$(CC) $(FLAGS) ./src/server/src/network/commands/client_connection_session_commands.cpp 

client_connection_session_network.o: ./src/server/src/network/commands/client_connection_session_network.cpp
	$(CC) $(FLAGS) ./src/server/src/network/commands/client_connection_session_network.cpp 

server.o: ./src/server/src/server.cpp
	$(CC) $(FLAGS) ./src/server/src/server.cpp 

server_handler_new_sessions.o: ./src/server/src/server_handler_new_sessions.cpp
	$(CC) $(FLAGS) ./src/server/src/server_handler_new_sessions.cpp 


application.o: ./src/client/src/application.cpp
	$(CC) $(FLAGS) ./src/client/src/application.cpp 

cmain.o: ./src/client/src/main.cpp
	$(CC) $(FLAGS) ./src/client/src/main.cpp -o cmain.o

client_network_handler.o: ./src/client/src/network/client_network_handler.cpp
	$(CC) $(FLAGS) ./src/client/src/network/client_network_handler.cpp 

network_commands_client.o: ./src/client/src/network/network_commands_client.cpp
	$(CC) $(FLAGS) ./src/client/src/network/network_commands_client.cpp 

network_commands_server.o: ./src/client/src/network/network_commands_server.cpp
	$(CC) $(FLAGS) ./src/client/src/network/network_commands_server.cpp 

client_ui_handler.o: ./src/client/src/ui/client_ui_handler.cpp
	$(CC) $(FLAGS) ./src/client/src/ui/client_ui_handler.cpp 

handler.o: ./src/client/src/inotify/handler.cpp
	$(CC) $(FLAGS) ./src/client/src/inotify/handler.cpp

inotify_watcher.o: ./src/common/include/inotify_watcher.cpp
	$(CC) $(FLAGS) ./src/common/include/inotify_watcher.cpp 

async_cout.o: ./src/common/include/asyncio/async_cout.cpp
	$(CC) $(FLAGS) ./src/common/include/asyncio/async_cout.cpp 

user_interface.o: ./src/common/include/asyncio/user_interface.cpp
	$(CC) $(FLAGS) ./src/common/include/asyncio/user_interface.cpp 

logging.o: ./src/common/include/network/logging.cpp
	$(CC) $(FLAGS) ./src/common/include/network/logging.cpp 

connection_manager.o: ./src/common/include/network/connection_manager.cpp
	$(CC) $(FLAGS) ./src/common/include/network/connection_manager.cpp 

server_commands.o: ./src/common/include/network/server_commands.cpp
	$(CC) $(FLAGS) ./src/common/include/network/server_commands.cpp 

socket_commands.o: ./src/common/include/network/socket_commands.cpp
	$(CC) $(FLAGS) ./src/common/include/network/socket_commands.cpp 

client_commands.o: ./src/common/include/network/client_commands.cpp
	$(CC) $(FLAGS) ./src/common/include/network/client_commands.cpp 

packet.o: ./src/common/include/network/packet.cpp
	$(CC) $(FLAGS) ./src/common/include/network/packet.cpp 

utils.o: ./src/common/include/utils.cpp
	$(CC) $(FLAGS) ./src/common/include/utils.cpp 


clean:
	rm -f $(OBJS) $(OUT) $(OBJS0) $(OUT0) $(OBJS1) $(OUT1) $(OBJS2) $(OUT2)
