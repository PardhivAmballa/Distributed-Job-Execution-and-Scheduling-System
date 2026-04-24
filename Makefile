CC     = gcc
CFLAGS = -Wall -Wextra -Iinclude -pthread
LDFLAGS = -pthread

SERVER_SRC = server/server.c server/queue.c server/logger.c server/scheduler.c server/auth.c
SERVER_OBJ = $(SERVER_SRC:.c=.o)

CLIENT_SRC = client/client.c
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

WORKER_SRC = worker/worker.c
WORKER_OBJ = $(WORKER_SRC:.c=.o)

.PHONY: all clean

all: server_app client_app worker_app

server_app: $(SERVER_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

client_app: $(CLIENT_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

worker_app: $(WORKER_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f server_app client_app worker_app
	rm -f server/*.o client/*.o worker/*.o
	rm -f jobs.dat logs/system.log