#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include "../include/common.h"

#define PORT 8080

void execute_command(char *cmd, char *output) {
    int fd[2];
    pipe(fd);

    if (fork() == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);

        execl("/bin/sh", "sh", "-c", cmd, NULL);
        exit(1);
    } else {
        close(fd[1]);
        int n = read(fd[0], output, MAX_OUTPUT_LEN - 1);
        if (n > 0) {
            output[n] = '\0';
        }
        close(fd[0]);
        wait(NULL);
    }
}

void handle_client(int client_socket) {
    message_t msg;

    recv(client_socket, &msg, sizeof(msg), 0);

    execute_command(msg.job.command, msg.job.output);

    send(client_socket, &msg, sizeof(msg), 0);

    close(client_socket);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    printf("Server started (multi-client)...\n");

    while (1) {
        client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

        if (fork() == 0) {
            // Child process handles client
            close(server_fd);
            handle_client(client_socket);
            exit(0);
        } else {
            // Parent continues accepting new clients
            close(client_socket);
        }
    }

    close(server_fd);
    return 0;
}