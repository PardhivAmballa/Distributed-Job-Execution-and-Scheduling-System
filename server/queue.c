#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/common.h"
#include "../include/queue.h"

/*
 * NOTE: The caller (server.c) is responsible for holding queue_mutex
 * around all calls into this module to ensure thread safety (Bug 8 fix).
 */

job_t job_queue[MAX_JOBS];
int job_count = 0;

#define JOBS_FILE "jobs.dat"

/* Load jobs from disk on startup */
void load_jobs_from_file() {
    int fd = open(JOBS_FILE, O_RDONLY);
    if (fd < 0) return; /* File might not exist yet, which is fine */

    /* 4.2 File Locking: Use advisory read lock */
    struct flock fl;
    fl.l_type = F_RDLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0; /* Lock entire file */
    fcntl(fd, F_SETLKW, &fl);

    if (read(fd, &job_count, sizeof(int)) > 0) {
        read(fd, job_queue, sizeof(job_t) * job_count);
    }

    fl.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &fl);
    close(fd);
    printf("queue: loaded %d jobs from %s\n", job_count, JOBS_FILE);
}

/* Save jobs to disk to persist state */
void save_jobs_to_file() {
    int fd = open(JOBS_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("save_jobs_to_file: open");
        return;
    }

    /* 4.2 File Locking: Use advisory write lock */
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fcntl(fd, F_SETLKW, &fl);

    write(fd, &job_count, sizeof(int));
    write(fd, job_queue, sizeof(job_t) * job_count);

    fl.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &fl);
    close(fd);
}

/* Add job to queue. Returns the new job ID, or -1 if queue is full. */
int add_job(char *command) {
    if (job_count >= MAX_JOBS) {
        fprintf(stderr, "queue: job queue is full (MAX_JOBS=%d)\n", MAX_JOBS);
        return -1;
    }

    job_queue[job_count].job_id = job_count + 1;
    strncpy(job_queue[job_count].command, command, MAX_CMD_LEN - 1);
    job_queue[job_count].command[MAX_CMD_LEN - 1] = '\0';
    job_queue[job_count].status = JOB_PENDING;
    job_queue[job_count].output[0] = '\0';

    job_count++;
    save_jobs_to_file();
    return job_count; /* job_id == job_count after increment */
}

/*
 * Get the next pending job and mark it RUNNING.
 */
job_t *get_next_job() {
    for (int i = 0; i < job_count; i++) {
        if (job_queue[i].status == JOB_PENDING) {
            job_queue[i].status = JOB_RUNNING;
            save_jobs_to_file();
            return &job_queue[i];
        }
    }
    return NULL;
}

/* Update status and output of a job identified by job_id. */
void update_job(int job_id, job_status_t status, char *output) {
    for (int i = 0; i < job_count; i++) {
        if (job_queue[i].job_id == job_id) {
            job_queue[i].status = status;
            if (output != NULL) {
                strncpy(job_queue[i].output, output, MAX_OUTPUT_LEN - 1);
                job_queue[i].output[MAX_OUTPUT_LEN - 1] = '\0';
            }
            save_jobs_to_file();
            return;
        }
    }
    fprintf(stderr, "queue: update_job called with unknown job_id=%d\n", job_id);
}

/* Look up a job by its ID. Returns NULL if not found. */
job_t *get_job_by_id(int job_id) {
    for (int i = 0; i < job_count; i++) {
        if (job_queue[i].job_id == job_id) {
            return &job_queue[i];
        }
    }
    return NULL;
}