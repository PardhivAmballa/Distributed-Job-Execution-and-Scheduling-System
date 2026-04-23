#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../include/common.h"

#define PORT 8080

int main() {
    message_t msg;
    int sock;
    struct sockaddr_in server;

    // LOGIN
    printf("Username: ");
    scanf("%s", msg.username);
    printf("Password: ");
    scanf("%s", msg.password);

    msg.type = MSG_LOGIN;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    connect(sock, (struct sockaddr*)&server, sizeof(server));

    send(sock, &msg, sizeof(msg), 0);
    recv(sock, &msg, sizeof(msg), 0);

    printf("%s\n", msg.job.output);
    close(sock);

    while (1) {
        printf("\n1.Submit 2.Status 3.Result 4.Exit\n");
        int ch;
        scanf("%d", &ch);

        sock = socket(AF_INET, SOCK_STREAM, 0);
        connect(sock, (struct sockaddr*)&server, sizeof(server));

        if (ch == 1) {
            msg.type = MSG_SUBMIT;
            getchar();
            printf("Command: ");
            fgets(msg.job.command, MAX_CMD_LEN, stdin);
        }

        else if (ch == 2) {
            msg.type = MSG_STATUS;
            printf("Job ID: ");
            scanf("%d", &msg.job.job_id);
        }

        else if (ch == 3) {
            msg.type = MSG_RESULT;
            printf("Job ID: ");
            scanf("%d", &msg.job.job_id);
        }

        else break;

        send(sock, &msg, sizeof(msg), 0);
        recv(sock, &msg, sizeof(msg), 0);

        printf("Response: %s\n", msg.job.output);
        close(sock);
    }
}