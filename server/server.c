#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include "../include/common.h"
#include "../include/queue.h"

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

    // Add job to queue
    int job_id = add_job(msg.job.command);

    if (job_id == -1) {
        strcpy(msg.job.output, "Queue Full!");
    } else {
        sprintf(msg.job.output, "Job submitted with ID: %d", job_id);
    }

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

    while(1){
        client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        if (fork() == 0) { // Child process handles client
            close(server_fd);
            handle_client(client_socket);
            close(client_socket);
            exit(0);
        } 
        else { // Parent process
            close(client_socket);
        }

        // JOB QUEUE EXECUTION
        job_t *job = get_next_job();
        if (job != NULL) {
            char output[MAX_OUTPUT_LEN] = {0};
            execute_command(job->command, output);
            update_job(job->job_id, JOB_COMPLETED, output);
            printf("Executed Job %d: %s\n", job->job_id, job->command);
        }
    }

    close(server_fd);
    return 0;
}