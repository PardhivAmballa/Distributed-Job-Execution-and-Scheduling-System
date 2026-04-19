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

    if(fork()==0){
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);

        execl("/bin/sh", "sh", "-c", cmd, NULL);
        exit(1);
    } 
    else{
        close(fd[1]);
        int n = read(fd[0], output, MAX_OUTPUT_LEN-1);
        if(n>0){
            output[n]='\0';  // null terminate
        }
        close(fd[0]);
        wait(NULL);
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    message_t msg;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    printf("Server started...\n");

    new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

    recv(new_socket, &msg, sizeof(msg), 0);

    execute_command(msg.job.command, msg.job.output);

    send(new_socket, &msg, sizeof(msg), 0);

    close(new_socket);
    close(server_fd);

    return 0;
}