#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>

#include "../include/common.h"
#include "../include/queue.h"
#include "../include/scheduler.h"
#include "../include/logger.h"
#include "../include/auth.h"

int port = 8080;

/* Mutex to protect the global job queue from concurrent access */
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 4.3 Concurrency Control: Thread Pool variables */
#define THREAD_POOL_SIZE 10
#define TASK_QUEUE_SIZE 100

int task_queue[TASK_QUEUE_SIZE];
int task_count = 0;
int task_front = 0;
int task_rear = 0;

pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t task_sem;

/* 4.1 Role-Based Authorization */
void handle_client_logic(int client_socket) {
    message_t msg;

    /* Check recv() return value */
    ssize_t n = recv(client_socket, &msg, sizeof(msg), 0);
    if (n <= 0) {
        close(client_socket);
        return;
    }

    char role[20] = {0};
    int authenticated = authenticate(msg.username, msg.password, role);

    if (msg.type == MSG_LOGIN) {
        if (authenticated) {
            strcpy(msg.job.output, "Login Success");
        } else {
            strcpy(msg.job.output, "Login Failed");
        }
    }
    else if (!authenticated) {
        strcpy(msg.job.output, "Unauthorized");
    }
    else if (msg.type == MSG_SUBMIT) {
        if (strcmp(role, "admin") != 0 && strcmp(role, "user") != 0) {
            strcpy(msg.job.output, "Unauthorized: Role not allowed");
        } else {
            pthread_mutex_lock(&queue_mutex);
            int id = add_job(msg.job.command);
            pthread_mutex_unlock(&queue_mutex);
            
            if (id == -1) {
                strcpy(msg.job.output, "Error: Queue is full");
            } else {
                sprintf(msg.job.output, "Job ID: %d", id);
            }
        }
    }
    else if (msg.type == MSG_STATUS) {
        if (strcmp(role, "admin") != 0 && strcmp(role, "user") != 0) {
            strcpy(msg.job.output, "Unauthorized: Role not allowed");
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
        if (strcmp(role, "admin") != 0 && strcmp(role, "user") != 0) {
            strcpy(msg.job.output, "Unauthorized: Role not allowed");
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
        /* Workers use MSG_ASSIGN */
        if (strcmp(role, "worker") != 0 && strcmp(role, "admin") != 0) {
            strcpy(msg.job.output, "Unauthorized: Only workers can assign");
            msg.job.job_id = -1;
        } else {
            pthread_mutex_lock(&queue_mutex);
            job_t *job = schedule_job();
            if (!job) {
                msg.job.job_id = -1;
                strcpy(msg.job.output, "No pending jobs");
            } else {
                msg.job = *job;
            }
            pthread_mutex_unlock(&queue_mutex);
        }
    }
    else if (msg.type == MSG_COMPLETE) {
        if (strcmp(role, "worker") != 0 && strcmp(role, "admin") != 0) {
            strcpy(msg.job.output, "Unauthorized: Only workers can complete");
        } else {
            pthread_mutex_lock(&queue_mutex);
            update_job(msg.job.job_id, JOB_COMPLETED, msg.job.output);
            pthread_mutex_unlock(&queue_mutex);
            log_job(msg.job.job_id, msg.job.command, "COMPLETED");
            strcpy(msg.job.output, "ACK");
        }
    }

    send(client_socket, &msg, sizeof(msg), 0);
    close(client_socket);
}

/* 4.3 Concurrency Control: Thread worker function */
void *thread_worker(void *arg) {
    (void)arg;
    while (1) {
        sem_wait(&task_sem);
        pthread_mutex_lock(&task_mutex);
        int client_socket = task_queue[task_front];
        task_front = (task_front + 1) % TASK_QUEUE_SIZE;
        task_count--;
        pthread_mutex_unlock(&task_mutex);

        handle_client_logic(client_socket);
    }
    return NULL;
}

void submit_task(int client_socket) {
    pthread_mutex_lock(&task_mutex);
    if (task_count < TASK_QUEUE_SIZE) {
        task_queue[task_rear] = client_socket;
        task_rear = (task_rear + 1) % TASK_QUEUE_SIZE;
        task_count++;
        sem_post(&task_sem);
    } else {
        close(client_socket);
        fprintf(stderr, "Server: Task queue full, dropping connection\n");
    }
    pthread_mutex_unlock(&task_mutex);
}

void print_help() {
    printf("Usage: ./server_app [OPTIONS]\n");
    printf("Options:\n");
    printf("  -h          Print this help message\n");
    printf("  -p PORT     Start server on specified port (default 8080)\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "hp:")) != -1) {
        switch (opt) {
            case 'h':
                print_help();
                break;
            case 'p':
                port = atoi(optarg);
                break;
            default:
                print_help();
        }
    }

    /* Load jobs from persistence storage */
    load_jobs_from_file();

    /* 4.3 Concurrency Control: Initialize Semaphore and Thread Pool */
    sem_init(&task_sem, 0, 0);
    pthread_t pool[THREAD_POOL_SIZE];
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        if (pthread_create(&pool[i], NULL, thread_worker, NULL) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }

    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int socket_opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &socket_opt, sizeof(socket_opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d with Thread Pool...\n", port);

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
            int client_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
            if (client_socket < 0) {
                perror("accept");
                continue;
            }

            /* Pass the socket to the thread pool */
            submit_task(client_socket);
        }
    }

    close(server_fd);
    sem_destroy(&task_sem);
    return 0;
}