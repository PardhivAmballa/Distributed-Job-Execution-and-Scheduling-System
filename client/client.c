#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../include/common.h"

#define PORT 8080

// Function to connect to the server
int connect_server(struct sockaddr_in *server) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    server->sin_family = AF_INET;
    server->sin_port = htons(PORT);
    server->sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(sock, (struct sockaddr*)server, sizeof(*server)) < 0){
        perror("Connection failed");
        return -1;
    }

    return sock;
}

// Main function
int main() {
    message_t msg;
    struct sockaddr_in server;

    char user[50], pass[50];

    printf("Username: ");
    scanf("%49s", user);

    printf("Password: ");
    scanf("%49s", pass);

    strcpy(msg.username, user);
    strcpy(msg.password, pass);

    msg.type = MSG_LOGIN;

    int sock = connect_server(&server); // Establishing connection with the server
    if (sock < 0) return 1;

    send(sock, &msg, sizeof(msg), 0); // Sending the login message to the server
    if (recv(sock, &msg, sizeof(msg), MSG_WAITALL) <= 0) {
        printf("\n[!] Connection to server lost.\n");
        close(sock);
        return 1;
    }

    printf("\n[✔] %s\n", msg.job.output);
    close(sock);

    int is_admin = (strcmp(user,"admin") == 0);

    while (1) {
        printf("\n=============================\n");
        printf(" Distributed Job System\n");
        printf("=============================\n");
        printf("1. Submit Job\n");
        printf("2. Check Job Status\n");
        printf("3. Get Job Result\n");
        if (is_admin) printf("4. View Logs\n5. Exit\n");
        else printf("4. Exit\n");
        printf("=============================\n");

        printf("Enter choice: ");
        int ch;
        if (scanf("%d", &ch) != 1) {
            while (getchar() != '\n'); // clear input buffer
            printf("Invalid input. Please enter a number.\n");
            continue;
        }

        memset(&msg, 0, sizeof(msg)); // Zeroing out the message structure
        strcpy(msg.username, user); // Copying the username to the message structure
        strcpy(msg.password, pass); // Copying the password to the message structure

        sock = connect_server(&server); // Establishing connection with the server
        if (sock < 0) break;

        if (ch == 1) {
            msg.type = MSG_SUBMIT;
            getchar();
            printf("Enter command: ");
            fgets(msg.job.command, MAX_CMD_LEN, stdin);
            msg.job.command[strcspn(msg.job.command, "\n")] = 0;

            if (strlen(msg.job.command) == 0) {
                printf("Error: Command cannot be empty.\n");
                continue;
            }
        }

        else if (ch == 2) {
            msg.type = MSG_STATUS;
            printf("Job ID: ");
            if (scanf("%d", &msg.job.job_id) != 1) {
                while (getchar() != '\n'); // clear input buffer
                printf("Invalid Job ID. Please enter a number.\n");
                continue;
            }
        }

        else if (ch == 3) {
            msg.type = MSG_RESULT;
            printf("Job ID: ");
            if (scanf("%d", &msg.job.job_id) != 1) {
                while (getchar() != '\n'); // clear input buffer
                printf("Invalid Job ID. Please enter a number.\n");
                continue;
            }
        }

        else if (is_admin && ch == 4) {
            printf("\n===== SYSTEM LOGS =====\n");

            FILE *fp = fopen("logs/system.log", "r");

            if(!fp){ printf("No logs found.\n");} 
            else {
                char line[256];
                while (fgets(line, sizeof(line), fp)) {printf("%s", line);}
                fclose(fp);
            }

            printf("========================\n");
            close(sock);
            continue;
        }

        else if ((is_admin && ch == 5) || (!is_admin && ch == 4)) {
            printf("Exiting...\n");
            close(sock);
            break;
        }

        else {
            printf("Invalid choice\n");
            close(sock);
            continue;
        }

        send(sock, &msg, sizeof(msg), 0);
        if (recv(sock, &msg, sizeof(msg), MSG_WAITALL) <= 0) {
            printf("\n[!] Connection to server lost.\n");
            close(sock);
            break;
        }

        printf("\n[+] %s\n", msg.job.output);

        close(sock);
    }

    return 0;
}