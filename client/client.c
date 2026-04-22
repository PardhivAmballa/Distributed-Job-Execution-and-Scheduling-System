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

    while (1) {
        printf("\n==== Job System ====\n");
        printf("1. Submit Job\n");
        printf("2. Exit\n");
        printf("Enter choice: ");

        int choice;
        scanf("%d", &choice);
        getchar(); // clear newline

        if (choice == 1) {
            sock = socket(AF_INET, SOCK_STREAM, 0);

            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(PORT);
            server_addr.sin_addr.s_addr = INADDR_ANY;

            connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));

            printf("Enter command: ");
            fgets(msg.job.command, MAX_CMD_LEN, stdin);

            msg.type = 1;

            send(sock, &msg, sizeof(msg), 0);
            recv(sock, &msg, sizeof(msg), 0);

            printf("Response: %s\n", msg.job.output);

            close(sock);
        }
        else if (choice == 2) {
            break;
        }
        else {
            printf("Invalid choice\n");
        }
    }

    return 0;
}