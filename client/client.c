#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../include/common.h"

#define PORT 8080

int main() {
    int sock;
    struct sockaddr_in server_addr;
    message_t msg;

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // Set server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Connect to server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }

    printf("Enter command: ");
    // Read command from user
    fgets(msg.job.command, MAX_CMD_LEN, stdin);

    msg.type = 1; // submit job

    send(sock, &msg, sizeof(msg), 0); // Send job to server

    recv(sock, &msg, sizeof(msg), 0); // Receive output from server

    printf("Output:\n%s\n", msg.job.output);

    close(sock);
    return 0;
}