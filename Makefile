CC = gcc
CFLAGS = -Wall

SERVER_SRC = server/server.c server/queue.c server/logger.c server/scheduler.c
CLIENT_SRC = client/client.c

SERVER_OUT = server_app
CLIENT_OUT = client_app

all: $(SERVER_OUT) $(CLIENT_OUT)

$(SERVER_OUT):
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER_OUT)

$(CLIENT_OUT):
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_OUT)

run-server:
	./$(SERVER_OUT)

run-client:
	./$(CLIENT_OUT)

clean:
	rm -f $(SERVER_OUT) $(CLIENT_OUT)