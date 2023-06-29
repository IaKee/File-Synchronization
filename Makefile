CC = g++
CFLAGS = -std=c++17 -Iinclude
LDFLAGS = -lboost_system -lboost_thread -lpthread

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

CLIENT_SRC = $(SRC_DIR)/client/client.cpp
CLIENT_OBJ = $(OBJ_DIR)/client.o
CLIENT_EXECUTABLE = $(BIN_DIR)/client_executable
MKDIR_P := mkdir -p

SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*/*.cpp)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

client: $(CLIENT_EXECUTABLE)

$(CLIENT_EXECUTABLE): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(OBJ_DIR)/client.o: $(CLIENT_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_DIR)/*.o $(EXECUTABLE)

run_client: $(CLIENT_EXECUTABLE)
	@./$(CLIENT_EXECUTABLE)