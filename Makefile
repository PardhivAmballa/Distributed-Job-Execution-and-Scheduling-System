CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -pthread
LDFLAGS = -pthread

BUILD_DIR = build

SERVER_SRC = server/server.c server/queue.c server/logger.c server/scheduler.c server/auth.c
CLIENT_SRC = client/client.c

SERVER_OBJ = $(SERVER_SRC:%.c=$(BUILD_DIR)/%.o)
CLIENT_OBJ = $(CLIENT_SRC:%.c=$(BUILD_DIR)/%.o)

SERVER_OUT = server_app
CLIENT_OUT = client_app

.PHONY: all clean

all: directories $(SERVER_OUT) $(CLIENT_OUT)

directories:
	mkdir -p $(BUILD_DIR)/server
	mkdir -p $(BUILD_DIR)/client
	mkdir -p logs

$(SERVER_OUT): $(SERVER_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

$(CLIENT_OUT): $(CLIENT_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build
	rm -f server_app client_app
	rm -f logs/system.log
	rm -f jobs.dat server/jobs.dat