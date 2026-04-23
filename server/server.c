#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/wait.h>

#include "../include/common.h"
#include "../include/queue.h"
#include "../include/scheduler.h"
#include "../include/logger.h"
#include "../include/auth.h"

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
        if (n > 0) output[n] = '\0';
        close(fd[0]);
        wait(NULL);
    }
}

void handle_client(int client_socket) {
    message_t msg;
    recv(client_socket, &msg, sizeof(msg), 0);

    if (msg.type == MSG_LOGIN) {
        if (authenticate(msg.username, msg.password))
            strcpy(msg.job.output, "Login Success");
        else
            strcpy(msg.job.output, "Login Failed");
    }

    else if (msg.type == MSG_SUBMIT) {
        int id = add_job(msg.job.command);
        sprintf(msg.job.output, "Job ID: %d", id);
    }

    else if (msg.type == MSG_STATUS) {
        job_t *job = get_job_by_id(msg.job.job_id);
        if (!job) strcpy(msg.job.output, "Invalid ID");
        else sprintf(msg.job.output, "Status: %d", job->status);
    }

    else if (msg.type == MSG_RESULT) {
        job_t *job = get_job_by_id(msg.job.job_id);
        if (!job) strcpy(msg.job.output, "Invalid ID");
        else strcpy(msg.job.output, job->output);
    }

    else if (msg.type == MSG_ASSIGN) {
        job_t *job = get_next_job();
        if (!job) {
            msg.job.job_id = -1;
            strcpy(msg.job.output, "No pending jobs");
        } else {
            msg.job = *job;
        }
    }

    else if (msg.type == MSG_COMPLETE) {
        update_job(msg.job.job_id, JOB_COMPLETED, msg.job.output);
        log_job(msg.job.job_id, msg.job.command, "COMPLETED");
        strcpy(msg.job.output, "ACK");
    }

    send(client_socket, &msg, sizeof(msg), 0);
    close(client_socket);
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    printf("Server started...\n");

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

        struct timeval tv = {0, 100000};

        int activity = select(server_fd + 1, &readfds, NULL, NULL, &tv);

        if (activity > 0 && FD_ISSET(server_fd, &readfds)) {
            int client_socket = accept(server_fd,
                (struct sockaddr*)&address, &addrlen);
            handle_client(client_socket);
        }
    }
}