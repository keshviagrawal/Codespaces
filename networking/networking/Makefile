# Makefile for S.H.A.M. Protocol Implementation

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
LDFLAGS = -lcrypto

# Detect OS for OpenSSL paths
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS
    OPENSSL_PREFIX := $(shell brew --prefix openssl)
    CFLAGS += -I$(OPENSSL_PREFIX)/include
    LDFLAGS += -L$(OPENSSL_PREFIX)/lib -lcrypto
else
    # Linux
    LDFLAGS += -lcrypto
endif

# Source files
SERVER_SRC = server.c
CLIENT_SRC = client.c
HEADER = sham.h

# Object files
SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

# Executables
SERVER_EXEC = server
CLIENT_EXEC = client

# Default target
all: $(SERVER_EXEC) $(CLIENT_EXEC)

# Build server
$(SERVER_EXEC): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Build client
$(CLIENT_EXEC): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Compile object files
%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ) $(SERVER_EXEC) $(CLIENT_EXEC)
	rm -f server_log.txt client_log.txt received_file

# Install dependencies (macOS)
install-deps-mac:
	@echo "Installing OpenSSL via Homebrew..."
	brew install openssl

# Install dependencies (Linux)
install-deps-linux:
	@echo "Installing OpenSSL development libraries..."
	sudo apt update && sudo apt install libssl-dev

# Test file transfer
test-file: $(SERVER_EXEC) $(CLIENT_EXEC)
	@echo "Testing file transfer..."
	@echo "Hello, World!" > test_input.txt
	@echo "Starting server in background..."
	./$(SERVER_EXEC) 8080 &
	@sleep 2
	@echo "Starting client..."
	./$(CLIENT_EXEC) 127.0.0.1 8080 test_input.txt received_file
	@echo "File transfer test completed"
	@echo "Input file content:"
	@cat test_input.txt
	@echo "Received file content:"
	@cat received_file
	@echo "Cleaning up..."
	@pkill -f $(SERVER_EXEC) || true
	@rm -f test_input.txt received_file

# Test chat mode
test-chat: $(SERVER_EXEC) $(CLIENT_EXEC)
	@echo "Testing chat mode..."
	@echo "Starting server in background..."
	./$(SERVER_EXEC) 8080 --chat &
	@sleep 2
	@echo "Starting client..."
	@echo "Note: You can type messages and '/quit' to exit"
	./$(CLIENT_EXEC) 127.0.0.1 8080 --chat
	@echo "Chat test completed"
	@echo "Cleaning up..."
	@pkill -f $(SERVER_EXEC) || true

# Test with packet loss
test-loss: $(SERVER_EXEC) $(CLIENT_EXEC)
	@echo "Testing with packet loss (10%)..."
	@echo "Hello, World!" > test_input.txt
	@echo "Starting server in background..."
	./$(SERVER_EXEC) 8080 0.1 &
	@sleep 2
	@echo "Starting client..."
	./$(CLIENT_EXEC) 127.0.0.1 8080 test_input.txt received_file 0.1
	@echo "Packet loss test completed"
	@echo "Cleaning up..."
	@pkill -f $(SERVER_EXEC) || true
	@rm -f test_input.txt received_file

# Test with logging
test-log: $(SERVER_EXEC) $(CLIENT_EXEC)
	@echo "Testing with logging enabled..."
	@echo "Hello, World!" > test_input.txt
	@echo "Starting server in background..."
	RUDP_LOG=1 ./$(SERVER_EXEC) 8080 &
	@sleep 2
	@echo "Starting client..."
	RUDP_LOG=1 ./$(CLIENT_EXEC) 127.0.0.1 8080 test_input.txt received_file
	@echo "Logging test completed"
	@echo "Server log:"
	@cat server_log.txt || echo "No server log found"
	@echo "Client log:"
	@cat client_log.txt || echo "No client log found"
	@echo "Cleaning up..."
	@pkill -f $(SERVER_EXEC) || true
	@rm -f test_input.txt received_file

# Run all tests
test: test-file test-chat test-loss test-log

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build both server and client"
	@echo "  server       - Build server only"
	@echo "  client       - Build client only"
	@echo "  clean        - Remove build artifacts"
	@echo "  install-deps-mac   - Install dependencies on macOS"
	@echo "  install-deps-linux - Install dependencies on Linux"
	@echo "  test-file    - Test file transfer mode"
	@echo "  test-chat    - Test chat mode"
	@echo "  test-loss    - Test with packet loss"
	@echo "  test-log     - Test with logging enabled"
	@echo "  test         - Run all tests"
	@echo "  help         - Show this help message"

# Phony targets
.PHONY: all clean install-deps-mac install-deps-linux test-file test-chat test-loss test-log test help
