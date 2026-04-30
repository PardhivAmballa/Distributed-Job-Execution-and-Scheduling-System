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


int task_queue[TASK_QUEUE_SIZE]; // Task queue
int front = 0, rear = 0, count = 0; // Circular buffer for task queue

pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for task queue
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for job queue

sem_t task_sem; // Semaphore to signal task arrival


// Executes a command and stores the output
void execute_command(char *cmd, char *output) {
    int fd[2];
    if (pipe(fd) < 0) {
        perror("pipe");
        return;
    }

    if (fork() == 0) { // Child process
        dup2(fd[1], STDOUT_FILENO); // Redirect stdout to pipe
        close(fd[0]); // Close unused read end
        close(fd[1]); // Close write end

        execl("/bin/sh", "sh", "-c", cmd, NULL); // Execute command
        exit(1);
    } 
    else { // Parent process
        close(fd[1]); // Close write end
        int n = read(fd[0], output, MAX_OUTPUT_LEN - 1);
        if(n > 0) output[n] = '\0'; // Null-terminate output
        close(fd[0]); // Close read end
        wait(NULL); // Wait for child process to finish
    }
}


// Handles client connections
void handle_client(int client_socket) {
    message_t msg;

    if (recv(client_socket, &msg, sizeof(msg), 0) <= 0) {
        close(client_socket);
        return;
    }

    char role[20] = {0};
    int auth = authenticate(msg.username, msg.password, role);

    // Handles login
    if (msg.type == MSG_LOGIN) {
        strcpy(msg.job.output, auth ? "Login Success" : "Login Failed");
    }

    // Handles unauthorized access
    else if (!auth) {
        strcpy(msg.job.output, "Unauthorized");
    }

    // Handles job submission
    else if (msg.type == MSG_SUBMIT) {
        pthread_mutex_lock(&queue_mutex);
        int id = add_job(msg.job.command, msg.username);
        pthread_mutex_unlock(&queue_mutex);

        if (id == -1)
            strcpy(msg.job.output, "Queue Full");
        else
            sprintf(msg.job.output, "Job ID: %d", id);
    }

    // Handles job status
    else if (msg.type == MSG_STATUS) {
        pthread_mutex_lock(&queue_mutex);
        job_t *job = get_job_by_id(msg.job.job_id);
        pthread_mutex_unlock(&queue_mutex);

        if (!job) {
            strcpy(msg.job.output, "Invalid ID");
        }
        else if (strcmp(role, "admin") != 0 && strcmp(job->owner, msg.username) != 0) {
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

    // Handles job result
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

    send(client_socket, &msg, sizeof(msg), 0);
    close(client_socket);
}


// Worker threads that handle client connections
void *worker_thread(void *arg) {
    (void)arg;
    while (1) { // Infinite loop to handle multiple client connections
        sem_wait(&task_sem); // Wait for a new task to arrive

        pthread_mutex_lock(&task_mutex);
        int client_socket = task_queue[front]; // Get the client socket
        front = (front + 1) % TASK_QUEUE_SIZE; // Move to the next task
        count--; // Decrement the waiting count
        pthread_mutex_unlock(&task_mutex);

        handle_client(client_socket);
    }
} // This is a Producer-Consumer pattern. The main process is the producer and the worker threads are the consumers.


// Executes jobs from the queue using the scheduling algorithm
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

        usleep(100000); // wait for 0.1 seconds to prevent busy waiting
    }
}


// Main server function that sets up the server and handles incoming connections
int main() {
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    load_jobs_from_file();

    sem_init(&task_sem, 0, 0); // Initialize the semaphore to 0 as there are no tasks initially

    pthread_t threads[THREAD_POOL_SIZE]; // Creating worker threads
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
        pthread_detach(threads[i]); // They run independently of the main thread and will automatically clean up when they exit
    }

    pthread_t exec_thread; // Creating executor thread
    pthread_create(&exec_thread, NULL, executor_thread, NULL);

    server_fd = socket(AF_INET, SOCK_STREAM, 0); // Creating the server socket

    address.sin_family = AF_INET; // IPv4 address family
    address.sin_addr.s_addr = INADDR_ANY; // Listen on any available network interface
    address.sin_port = htons(PORT); // Convert port number to network byte order

    bind(server_fd, (struct sockaddr*)&address, sizeof(address)); // Binding the socket to the address and port
    listen(server_fd, 10); // Listening for incoming connections, 10 is the maximum number of connections that can be queued

    printf("Server running on port %d...\n", PORT);

    while (1) {
        int client_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen); // Blocking call, waits for a client to connect

        pthread_mutex_lock(&task_mutex);

        if (count < TASK_QUEUE_SIZE) { // Checking if the task queue is full
            task_queue[rear] = client_socket; // Adding the client socket to the task queue
            rear = (rear + 1) % TASK_QUEUE_SIZE; // Moving to the next task
            count++; // Incrementing the task count
            sem_post(&task_sem);
        } else {
            close(client_socket);
        }

        pthread_mutex_unlock(&task_mutex);
    }

    close(server_fd);
    return 0;
}