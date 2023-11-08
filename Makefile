# Compile client source files into an executable named 'client'
client: 
	g++ -o client src/client/*.cpp src/include/common/*.cpp -lcryptopp -lpthread


# Compile server source files into an executable named 'server'
server:
	g++ -o server src/server/*.cpp src/include/common/*.cpp -lcryptopp -lpthread

# Target 'both' depends on both 'client' and 'server' executables
both: client server

# Remove previously compiled executables (client and server)
clean:
	rm -f client server

runclient:
	./client

runserver:
	./server

testserver: clean server runserver


# Run both server and client in separate gnome-terminals
urunboth:
	gnome-terminal -- bash -c './server; exec bash'
	gnome-terminal -- bash -c 'sleep 2; ./client iakee 0.0.0.0 65535; exec bash'

srunboth:
	gnome-terminal --geometry 80x24+0+0 -- bash -c './server; exec bash'
	gnome-terminal --geometry 80x24+0+400 -- bash -c 'sleep 2; ./client iakee 0.0.0.0 65535; exec bash'


# Install necessary packages and libraries
install:
	# Check if the system is Linux (including WSL)
	if [ -f /etc/lsb-release ]; then \
		# If Linux, install required packages using apt-get
		sudo apt-get install g++; \
		sudo apt-get install gcc; \
		sudo apt-get install libcrypto++-dev; \
	else \
		# If not Linux, add appropriate installation commands for the system here
		# For WSL, you might use 'apt' or other suitable package managers
		echo "Please install dependencies manually on non-Linux systems"; \
	fi