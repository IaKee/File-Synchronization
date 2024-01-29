MAKEFLAGS += -j5

OBJDIR  = build
BINDIR  = bin
TESTDIR = tests
CDIR    = $(TESTDIR)/clients
SDIR    = $(TESTDIR)/servers

OBJS	= $(OBJDIR)/main.o $(OBJDIR)/server_handler_user_interface.o $(OBJDIR)/client_connection_session.o $(OBJDIR)/server_connection_session.o $(OBJDIR)/client_connection_user.o $(OBJDIR)/server_connection_unit.o $(OBJDIR)/client_connection_group.o $(OBJDIR)/server_connection_group.o $(OBJDIR)/client_connection.o $(OBJDIR)/server_connection.o $(OBJDIR)/client_connection_session_commands.o $(OBJDIR)/server_connection_session_commands.o $(OBJDIR)/client_connection_session_network.o $(OBJDIR)/server_connection_session_network.o $(OBJDIR)/server.o $(OBJDIR)/server_handler_new_sessions.o 
OUT	    = $(BINDIR)/server,$(BINDIR)/client,$(OBJDIR)/common.o

OBJS0	= $(OBJDIR)/smain.o $(OBJDIR)/server_handler_user_interface.o $(OBJDIR)/client_connection_session.o $(OBJDIR)/server_connection_session.o $(OBJDIR)/client_connection_user.o $(OBJDIR)/server_connection_unit.o $(OBJDIR)/client_connection_group.o $(OBJDIR)/server_connection_group.o $(OBJDIR)/client_connection.o $(OBJDIR)/server_connection.o $(OBJDIR)/client_connection_session_commands.o $(OBJDIR)/server_connection_session_commands.o $(OBJDIR)/client_connection_session_network.o $(OBJDIR)/server_connection_session_network.o $(OBJDIR)/server.o $(OBJDIR)/server_handler_new_sessions.o
SOURCE0	= ./src/server/src/main.cpp ./src/server/src/server_handler_user_interface.cpp ./src/server/src/network/managers/client_connection_session.cpp ./src/server/src/network/managers/server_connection_session.cpp ./src/server/src/network/managers/client_connection_user.cpp ./src/server/src/network/managers/server_connection_unit.cpp ./src/server/src/network/managers/client_connection_group.cpp ./src/server/src/network/managers/server_connection_group.cpp ./src/server/src/network/client_connection.cpp ./src/server/src/network/server_connection.cpp ./src/server/src/network/commands/client_connection_session_commands.cpp ./src/server/src/network/commands/server_connection_session_commands.cpp ./src/server/src/network/commands/client_connection_session_network.cpp ./src/server/src/network/commands/server_connection_session_network.cpp ./src/server/src/server.cpp ./src/server/src/server_handler_new_sessions.cpp ./src/server/tests/ex.cpp
HEADER0	= 
DEP0 	= 
OUT0	= $(BINDIR)/server

OBJS1	= $(OBJDIR)/application.o $(OBJDIR)/cmain.o $(OBJDIR)/client_network_handler.o $(OBJDIR)/network_commands_client.o $(OBJDIR)/network_commands_server.o $(OBJDIR)/client_ui_handler.o $(OBJDIR)/handler.o 
SOURCE1	= ./src/client/src/application.cpp ./src/client/src/main.cpp ./src/client/src/network/client_network_handler.cpp ./src/client/src/network/network_commands_client.cpp ./src/client/src/network/network_commands_server.cpp ./src/client/src/ui/client_ui_handler.cpp ./src/client/src/inotify/handler.cpp
HEADER1	= 
DEP1 	= 
OUT1	= $(BINDIR)/client

OBJS2	= $(OBJDIR)/inotify_watcher.o $(OBJDIR)/async_cout.o $(OBJDIR)/user_interface.o $(OBJDIR)/logging.o $(OBJDIR)/connection_manager.o $(OBJDIR)/server_commands.o $(OBJDIR)/socket_commands.o $(OBJDIR)/client_commands.o $(OBJDIR)/packet.o $(OBJDIR)/utils.o 
SOURCE2	= ./src/common/include/inotify_watcher.cpp ./src/common/include/asyncio/async_cout.cpp ./src/common/include/asyncio/user_interface.cpp ./src/common/include/network/logging.cpp ./src/common/include/network/connection_manager.cpp ./src/common/include/network/server_commands.cpp ./src/common/include/network/socket_commands.cpp ./src/common/include/network/client_commands.cpp ./src/common/include/network/packet.cpp ./src/common/include/utils.cpp
HEADER2	= 
DEP2 	=
OUT2	= $(OBJDIR)/common.o

CC	    = g++
FLAGS	= -g -c -Wall
LFLAGS	= -lpthread

all: $(BINDIR)/server $(BINDIR)/client 

client1: $(CDIR) $(BINDIR)/client
	cd $(CDIR)/client1; \
	../../../$(BINDIR)/client iakee 0.0.0.0 65535;

client2: $(CDIR) $(BINDIR)/client
	cd $(CDIR)/client2; \
	../../../$(BINDIR)/client iakee 0.0.0.0 65535;

server1: $(SDIR) $(BINDIR)/server
	cd $(SDIR)/server1; \
	../../../$(BINDIR)/server;

server2: $(SDIR) $(BINDIR)/server
	cd $(SDIR)/server2; \
	../../../$(BINDIR)/server;

server3: $(SDIR) $(BINDIR)/server
	cd $(SDIR)/server3; \
	../../../$(BINDIR)/server;

test: $(CDIR) $(SDIR) all
	tmux new-session -s "server" -d "cd $(SDIR)/server1 && ../../../$(BINDIR)/server; read"
	tmux split-window -h "sleep 2 && cd $(CDIR)/client1 && ../../../$(BINDIR)/client iakee 0.0.0.0 65535; read"
	tmux -2 attach-session -d

$(BINDIR)/server: $(BINDIR) $(OBJDIR) $(OBJS0) $(LFLAGS) $(DEP0) $(OBJS2) 
	$(CC) -g $(OBJS0) $(OBJS2) -o $(OUT0) $(LFLAGS)

$(BINDIR)/client: $(BINDIR) $(OBJDIR) $(OBJS1) $(LFLAGS) $(DEP1) $(OBJS2) 
	$(CC) -g $(OBJS1) $(OBJS2) -o $(OUT1) $(LFLAGS)

# common: $(OBJS2) $(LFLAGS) $(DEP2) $(OBJDIR)
# 	$(CC) -g $(OBJS2) -o $(OUT2) $(LFLAGS)

$(BINDIR): 
	mkdir -p $@

$(OBJDIR):
	mkdir -p $@

$(TESTDIR):
	mkdir -p $@

$(CDIR): $(TESTDIR)
	mkdir -p $@
	mkdir -p $@/client1
	mkdir -p $@/client2

$(SDIR): $(TESTDIR)
	mkdir -p $@
	mkdir -p $@/server1
	mkdir -p $@/server2
	mkdir -p $@/server3

$(OBJDIR)/smain.o: ./src/server/src/main.cpp
	$(CC) $(FLAGS) ./src/server/src/main.cpp -o $@

$(OBJDIR)/server_handler_user_interface.o: ./src/server/src/server_handler_user_interface.cpp
	$(CC) $(FLAGS) ./src/server/src/server_handler_user_interface.cpp -o $@

$(OBJDIR)/client_connection_session.o: ./src/server/src/network/managers/client_connection_session.cpp
	$(CC) $(FLAGS) ./src/server/src/network/managers/client_connection_session.cpp -o $@

$(OBJDIR)/server_connection_session.o: ./src/server/src/network/managers/server_connection_session.cpp
	$(CC) $(FLAGS) ./src/server/src/network/managers/server_connection_session.cpp -o $@

$(OBJDIR)/client_connection_user.o: ./src/server/src/network/managers/client_connection_user.cpp
	$(CC) $(FLAGS) ./src/server/src/network/managers/client_connection_user.cpp -o $@

$(OBJDIR)/server_connection_unit.o: ./src/server/src/network/managers/server_connection_unit.cpp
	$(CC) $(FLAGS) ./src/server/src/network/managers/server_connection_unit.cpp -o $@

$(OBJDIR)/client_connection_group.o: ./src/server/src/network/managers/client_connection_group.cpp
	$(CC) $(FLAGS) ./src/server/src/network/managers/client_connection_group.cpp -o $@

$(OBJDIR)/server_connection_group.o: ./src/server/src/network/managers/server_connection_group.cpp
	$(CC) $(FLAGS) ./src/server/src/network/managers/server_connection_group.cpp -o $@

$(OBJDIR)/client_connection.o: ./src/server/src/network/client_connection.cpp
	$(CC) $(FLAGS) ./src/server/src/network/client_connection.cpp -o $@

$(OBJDIR)/server_connection.o: ./src/server/src/network/server_connection.cpp
	$(CC) $(FLAGS) ./src/server/src/network/server_connection.cpp -o $@

$(OBJDIR)/client_connection_session_commands.o: ./src/server/src/network/commands/client_connection_session_commands.cpp
	$(CC) $(FLAGS) ./src/server/src/network/commands/client_connection_session_commands.cpp -o $@

$(OBJDIR)/client_connection_session_network.o: ./src/server/src/network/commands/client_connection_session_network.cpp
	$(CC) $(FLAGS) ./src/server/src/network/commands/client_connection_session_network.cpp -o $@

$(OBJDIR)/server_connection_session_commands.o: ./src/server/src/network/commands/server_connection_session_commands.cpp
	$(CC) $(FLAGS) ./src/server/src/network/commands/server_connection_session_commands.cpp -o $@

$(OBJDIR)/server_connection_session_network.o: ./src/server/src/network/commands/server_connection_session_network.cpp
	$(CC) $(FLAGS) ./src/server/src/network/commands/server_connection_session_network.cpp -o $@

$(OBJDIR)/server.o: ./src/server/src/server.cpp
	$(CC) $(FLAGS) ./src/server/src/server.cpp -o $@

$(OBJDIR)/server_handler_new_sessions.o: ./src/server/src/server_handler_new_sessions.cpp
	$(CC) $(FLAGS) ./src/server/src/server_handler_new_sessions.cpp -o $@

$(OBJDIR)/application.o: ./src/client/src/application.cpp
	$(CC) $(FLAGS) ./src/client/src/application.cpp -o $@

$(OBJDIR)/cmain.o: ./src/client/src/main.cpp
	$(CC) $(FLAGS) ./src/client/src/main.cpp -o $@

$(OBJDIR)/client_network_handler.o: ./src/client/src/network/client_network_handler.cpp
	$(CC) $(FLAGS) ./src/client/src/network/client_network_handler.cpp -o $@

$(OBJDIR)/network_commands_client.o: ./src/client/src/network/network_commands_client.cpp
	$(CC) $(FLAGS) ./src/client/src/network/network_commands_client.cpp -o $@

$(OBJDIR)/network_commands_server.o: ./src/client/src/network/network_commands_server.cpp
	$(CC) $(FLAGS) ./src/client/src/network/network_commands_server.cpp -o $@

$(OBJDIR)/client_ui_handler.o: ./src/client/src/ui/client_ui_handler.cpp
	$(CC) $(FLAGS) ./src/client/src/ui/client_ui_handler.cpp -o $@

$(OBJDIR)/handler.o: ./src/client/src/inotify/handler.cpp
	$(CC) $(FLAGS) ./src/client/src/inotify/handler.cpp -o $@

$(OBJDIR)/inotify_watcher.o: ./src/common/include/inotify_watcher.cpp
	$(CC) $(FLAGS) ./src/common/include/inotify_watcher.cpp -o $@

$(OBJDIR)/async_cout.o: ./src/common/include/asyncio/async_cout.cpp
	$(CC) $(FLAGS) ./src/common/include/asyncio/async_cout.cpp -o $@

$(OBJDIR)/user_interface.o: ./src/common/include/asyncio/user_interface.cpp
	$(CC) $(FLAGS) ./src/common/include/asyncio/user_interface.cpp -o $@

$(OBJDIR)/logging.o: ./src/common/include/network/logging.cpp
	$(CC) $(FLAGS) ./src/common/include/network/logging.cpp -o $@

$(OBJDIR)/connection_manager.o: ./src/common/include/network/connection_manager.cpp
	$(CC) $(FLAGS) ./src/common/include/network/connection_manager.cpp -o $@

$(OBJDIR)/server_commands.o: ./src/common/include/network/server_commands.cpp
	$(CC) $(FLAGS) ./src/common/include/network/server_commands.cpp -o $@

$(OBJDIR)/socket_commands.o: ./src/common/include/network/socket_commands.cpp
	$(CC) $(FLAGS) ./src/common/include/network/socket_commands.cpp -o $@

$(OBJDIR)/client_commands.o: ./src/common/include/network/client_commands.cpp
	$(CC) $(FLAGS) ./src/common/include/network/client_commands.cpp -o $@

$(OBJDIR)/packet.o: ./src/common/include/network/packet.cpp
	$(CC) $(FLAGS) ./src/common/include/network/packet.cpp -o $@

$(OBJDIR)/utils.o: ./src/common/include/utils.cpp
	$(CC) $(FLAGS) ./src/common/include/utils.cpp -o $@

clean:
	rm -f $(OBJS) $(OUT) $(OBJS0) $(OUT0) $(OBJS1) $(OUT1) $(OBJS2) $(OUT2)
