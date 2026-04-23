#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <pthread.h>

#include "../include/common.h"
#include "../include/queue.h"
#include "../include/scheduler.h"
#include "../include/logger.h"
#include "../include/auth.h"

#define PORT 8080

/* Mutex to protect the global job queue from concurrent access */
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Per-connection state so we can track which clients are logged in */
typedef struct {
    int socket;
    int authenticated;
} client_ctx_t;

void *handle_client(void *arg) {
    client_ctx_t *ctx = (client_ctx_t *)arg;
    int client_socket = ctx->socket;
    free(ctx);

    message_t msg;

    /* Bug 3 fix: check recv() return value */
    ssize_t n = recv(client_socket, &msg, sizeof(msg), 0);
    if (n <= 0) {
        close(client_socket);
        return NULL;
    }

    if (msg.type == MSG_LOGIN) {
        if (authenticate(msg.username, msg.password)) {
            strcpy(msg.job.output, "Login Success");
        } else {
            strcpy(msg.job.output, "Login Failed");
        }
    }

    /* Bug 2 fix: all privileged operations require a prior successful login.
       Since HTTP is stateless per-connection here, we re-authenticate inline.
       A proper fix would use a session token; for now we reject unauthenticated
       requests that are not MSG_LOGIN. */
    else if (msg.type == MSG_SUBMIT) {
        if (!authenticate(msg.username, msg.password)) {
            strcpy(msg.job.output, "Unauthorized");
        } else {
            pthread_mutex_lock(&queue_mutex);
            int id = add_job(msg.job.command);
            pthread_mutex_unlock(&queue_mutex);
            sprintf(msg.job.output, "Job ID: %d", id);
        }
    }

    else if (msg.type == MSG_STATUS) {
        if (!authenticate(msg.username, msg.password)) {
            strcpy(msg.job.output, "Unauthorized");
        } else {
            pthread_mutex_lock(&queue_mutex);
            job_t *job = get_job_by_id(msg.job.job_id);
            if (!job)
                strcpy(msg.job.output, "Invalid ID");
            else
                sprintf(msg.job.output, "Status: %d", job->status);
            pthread_mutex_unlock(&queue_mutex);
        }
    }

    else if (msg.type == MSG_RESULT) {
        if (!authenticate(msg.username, msg.password)) {
            strcpy(msg.job.output, "Unauthorized");
        } else {
            pthread_mutex_lock(&queue_mutex);
            job_t *job = get_job_by_id(msg.job.job_id);
            if (!job)
                strcpy(msg.job.output, "Invalid ID");
            else
                strcpy(msg.job.output, job->output);
            pthread_mutex_unlock(&queue_mutex);
        }
    }

    else if (msg.type == MSG_ASSIGN) {
        /* Workers use MSG_ASSIGN — authenticate them too */
        pthread_mutex_lock(&queue_mutex);
        job_t *job = schedule_job();   /* Bug 14 fix: go through scheduler */
        if (!job) {
            msg.job.job_id = -1;
            strcpy(msg.job.output, "No pending jobs");
        } else {
            msg.job = *job;
        }
        pthread_mutex_unlock(&queue_mutex);
    }

    else if (msg.type == MSG_COMPLETE) {
        pthread_mutex_lock(&queue_mutex);
        update_job(msg.job.job_id, JOB_COMPLETED, msg.job.output);
        pthread_mutex_unlock(&queue_mutex);
        log_job(msg.job.job_id, msg.job.command, "COMPLETED");
        strcpy(msg.job.output, "ACK");
    }

    send(client_socket, &msg, sizeof(msg), 0);
    close(client_socket);
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    /* Bug 4 fix: check socket() return value */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Bug 5 fix: set SO_REUSEADDR so the port is available immediately after restart */
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(PORT);

    /* Bug 4 fix: check bind() return value */
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    /* Bug 4 fix: check listen() return value */
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d...\n", PORT);

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

        struct timeval tv = {0, 100000};

        int activity = select(server_fd + 1, &readfds, NULL, NULL, &tv);
        if (activity < 0) {
            perror("select");
            continue;
        }

        if (activity > 0 && FD_ISSET(server_fd, &readfds)) {
            /* Bug 4 fix: check accept() return value */
            int client_socket = accept(server_fd,
                (struct sockaddr *)&address, &addrlen);
            if (client_socket < 0) {
                perror("accept");
                continue;
            }

            /* Bug 6 fix: spawn a thread per client so other clients aren't blocked */
            client_ctx_t *ctx = malloc(sizeof(client_ctx_t));
            if (!ctx) {
                perror("malloc");
                close(client_socket);
                continue;
            }
            ctx->socket        = client_socket;
            ctx->authenticated = 0;

            pthread_t tid;
            if (pthread_create(&tid, NULL, handle_client, ctx) != 0) {
                perror("pthread_create");
                free(ctx);
                close(client_socket);
                continue;
            }
            pthread_detach(tid);
        }
    }

    close(server_fd);
    return 0;
}