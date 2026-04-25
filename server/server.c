#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h>

#include "../include/common.h"
#include "../include/queue.h"
#include "../include/logger.h"
#include "../include/auth.h"
#include "../include/scheduler.h"

#define PORT 8080
#define THREAD_POOL_SIZE 5
#define TASK_QUEUE_SIZE 100

/* ================= GLOBALS ================= */

int task_queue[TASK_QUEUE_SIZE];
int front = 0, rear = 0, count = 0;

pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

sem_t task_sem;

/* ================= EXECUTE ================= */

void execute_command(char *cmd, char *output) {
    int fd[2];
    if (pipe(fd) < 0) {
        perror("pipe");
        return;
    }

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

/* ================= CLIENT ================= */

void handle_client(int client_socket) {
    message_t msg;

    if (recv(client_socket, &msg, sizeof(msg), 0) <= 0) {
        close(client_socket);
        return;
    }

    char role[20] = {0};
    int auth = authenticate(msg.username, msg.password, role);

    /* LOGIN */
    if (msg.type == MSG_LOGIN) {
        strcpy(msg.job.output, auth ? "Login Success" : "Login Failed");
    }

    /* UNAUTHORIZED */
    else if (!auth) {
        strcpy(msg.job.output, "Unauthorized");
    }

    /* SUBMIT */
    else if (msg.type == MSG_SUBMIT) {
        pthread_mutex_lock(&queue_mutex);
        int id = add_job(msg.job.command, msg.username);
        pthread_mutex_unlock(&queue_mutex);

        if (id == -1)
            strcpy(msg.job.output, "Queue Full");
        else
            sprintf(msg.job.output, "Job ID: %d", id);
    }

    /* STATUS */
    else if (msg.type == MSG_STATUS) {
        pthread_mutex_lock(&queue_mutex);
        job_t *job = get_job_by_id(msg.job.job_id);
        pthread_mutex_unlock(&queue_mutex);

        if (!job) {
            strcpy(msg.job.output, "Invalid ID");
        }
        else if (strcmp(role, "admin") != 0 &&
                 strcmp(job->owner, msg.username) != 0) {
            strcpy(msg.job.output, "Access Denied");
        }
        else {
            char *status;
            switch (job->status) {
                case JOB_PENDING: status = "PENDING"; break;
                case JOB_RUNNING: status = "RUNNING"; break;
                case JOB_COMPLETED: status = "COMPLETED"; break;
                default: status = "UNKNOWN";
            }
            sprintf(msg.job.output, "Status: %s", status);
        }
    }

    /* RESULT */
    else if (msg.type == MSG_RESULT) {
        pthread_mutex_lock(&queue_mutex);
        job_t *job = get_job_by_id(msg.job.job_id);
        pthread_mutex_unlock(&queue_mutex);

        if (!job) {
            strcpy(msg.job.output, "Invalid ID");
        }
        else if (strcmp(role, "admin") != 0 &&
                 strcmp(job->owner, msg.username) != 0) {
            strcpy(msg.job.output, "Access Denied");
        }
        else {
            strcpy(msg.job.output, job->output);
        }
    }

    /* ADMIN LOGS */
    else if (msg.type == MSG_LOGS) {
        if (strcmp(role, "admin") != 0) {
            strcpy(msg.job.output, "Access Denied");
        } else {
            strcpy(msg.job.output, "Check logs/system.log");
        }
    }

    send(client_socket, &msg, sizeof(msg), 0);
    close(client_socket);
}

/* ================= THREAD ================= */

void *worker_thread(void *arg) {
    (void)arg;
    while (1) {
        sem_wait(&task_sem);

        pthread_mutex_lock(&task_mutex);
        int client_socket = task_queue[front];
        front = (front + 1) % TASK_QUEUE_SIZE;
        count--;
        pthread_mutex_unlock(&task_mutex);

        handle_client(client_socket);
    }
}

/* ================= EXECUTOR ================= */

void *executor_thread(void *arg) {
    (void)arg;
    while (1) {
        pthread_mutex_lock(&queue_mutex);
        job_t *job = schedule_job();
        pthread_mutex_unlock(&queue_mutex);

        if (job) {
            char output[MAX_OUTPUT_LEN] = {0};

            job->status = JOB_RUNNING;
            execute_command(job->command, output);

            pthread_mutex_lock(&queue_mutex);
            update_job(job->job_id, JOB_COMPLETED, output);
            pthread_mutex_unlock(&queue_mutex);

            log_job(job->job_id, job->command, "COMPLETED");

            printf("[EXEC] Job %d done: %s\n", job->job_id, job->command);
        }

        usleep(100000);
    }
}

/* ================= MAIN ================= */

int main() {
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    load_jobs_from_file();

    sem_init(&task_sem, 0, 0);

    pthread_t threads[THREAD_POOL_SIZE];
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
        pthread_detach(threads[i]);
    }

    pthread_t exec_thread;
    pthread_create(&exec_thread, NULL, executor_thread, NULL);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 10);

    printf("Server running on port %d...\n", PORT);

    while (1) {
        int client_socket = accept(server_fd,
            (struct sockaddr*)&address, &addrlen);

        pthread_mutex_lock(&task_mutex);

        if (count < TASK_QUEUE_SIZE) {
            task_queue[rear] = client_socket;
            rear = (rear + 1) % TASK_QUEUE_SIZE;
            count++;
            sem_post(&task_sem);
        } else {
            close(client_socket);
        }

        pthread_mutex_unlock(&task_mutex);
    }

    close(server_fd);
    return 0;
}