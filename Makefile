all:
	gcc server/server.c server/queue.c server/logger.c server/scheduler.c server/auth.c -o server_app
	gcc client/client.c -o client_app

clean:
	rm -f server_app client_app