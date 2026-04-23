CC     = gcc
CFLAGS = -Wall -Wextra -Iinclude

all: server_app client_app worker_app

server_app:
	$(CC) $(CFLAGS) server/server.c server/queue.c server/logger.c \
	    server/scheduler.c server/auth.c -o server_app -lpthread

client_app:
	$(CC) $(CFLAGS) client/client.c -o client_app

worker_app:
	$(CC) $(CFLAGS) worker/worker.c -o worker_app

clean:
	rm -f server_app client_app worker_app