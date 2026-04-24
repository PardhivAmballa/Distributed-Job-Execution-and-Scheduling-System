#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include "../include/common.h"

#define PORT 8080
#define SERVER_IP "127.0.0.1"

static int connect_to_server() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in server;
    server.sin_family      = AF_INET;
    server.sin_port        = htons(PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        close(sock);
        return -1;
    }
    return sock;
}

static void execute_command(char *cmd, char *output) {
    /* 4.6 Inter-Process Communication (IPC): Pipes
       Here we use an unnamed pipe to communicate between the parent (worker)
       and the child (shell executing the command). The child writes the command's
       stdout to the pipe, and the parent reads it to send back to the server. */
    int fd[2];
    pipe(fd);

    if (fork() == 0) {
        /* Child process */
        dup2(fd[1], STDOUT_FILENO); /* Redirect stdout to write end of pipe */
        close(fd[0]);
        close(fd[1]);
        execl("/bin/sh", "sh", "-c", cmd, NULL);
        exit(1);
    } else {
        /* Parent process */
        close(fd[1]);
        int n = read(fd[0], output, MAX_OUTPUT_LEN - 1);
        if (n > 0) output[n] = '\0';
        close(fd[0]);
        wait(NULL);
    }
}

int main() {
    printf("Worker started...\n");

    while (1) {
        /* --- Ask server for the next pending job --- */
        int sock = connect_to_server();
        if (sock < 0) {
            fprintf(stderr, "Worker: cannot connect to server, retrying...\n");
            sleep(2);
            continue;
        }

        message_t msg;
        memset(&msg, 0, sizeof(msg));
        msg.type = MSG_ASSIGN;
        /* Authenticate as worker role */
        strcpy(msg.username, "worker");
        strcpy(msg.password, "worker123");

        send(sock, &msg, sizeof(msg), 0);
        recv(sock, &msg, sizeof(msg), 0);
        close(sock);

        if (msg.job.job_id <= 0) {
            /* No pending jobs right now */
            usleep(200000);
            continue;
        }

        printf("Worker executing Job %d: %s\n", msg.job.job_id, msg.job.command);

        /* --- Execute the command --- */
        char output[MAX_OUTPUT_LEN] = {0};
        execute_command(msg.job.command, output);

        /* --- Report completion back to server --- */
        sock = connect_to_server();
        if (sock < 0) {
            fprintf(stderr, "Worker: cannot report completion for Job %d\n", msg.job.job_id);
            continue;
        }

        message_t result;
        memset(&result, 0, sizeof(result));
        result.type       = MSG_COMPLETE;
        result.job        = msg.job;
        /* Authenticate as worker role */
        strcpy(result.username, "worker");
        strcpy(result.password, "worker123");
        strncpy(result.job.output, output, MAX_OUTPUT_LEN - 1);

        send(sock, &result, sizeof(result), 0);
        recv(sock, &result, sizeof(result), 0);
        close(sock);

        printf("Job %d completed.\n", msg.job.job_id);
    }

    return 0;
}