#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "../include/common.h"
#include "../include/queue.h"

#define JOBS_FILE "jobs.dat"

job_t job_queue[MAX_JOBS];
int job_count = 0;

/* Load jobs from disk */
void load_jobs_from_file() {
    int fd = open(JOBS_FILE, O_RDONLY);
    if (fd < 0) return;

    struct flock fl = {
        .l_type = F_RDLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0,
        .l_pid = 0
    };
    
    fcntl(fd, F_SETLKW, &fl);

    read(fd, &job_count, sizeof(int));
    read(fd, job_queue, sizeof(job_t) * job_count);

    fl.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &fl);

    close(fd);

    printf("[INIT] Loaded %d jobs from disk\n", job_count);
}

/* Save jobs to disk */
void save_jobs_to_file() {
    int fd = open(JOBS_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("save_jobs");
        return;
    }

    struct flock fl = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0,
        .l_pid = 0
    };

    fcntl(fd, F_SETLKW, &fl);

    write(fd, &job_count, sizeof(int));
    write(fd, job_queue, sizeof(job_t) * job_count);

    fl.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &fl);

    close(fd);
}

/* Add job */
int add_job(char *command, char *username) {
    if (job_count >= MAX_JOBS) return -1;

    job_queue[job_count].job_id = job_count + 1;
    strcpy(job_queue[job_count].command, command);
    strcpy(job_queue[job_count].owner, username);
    job_queue[job_count].status = JOB_PENDING;

    job_count++;
    return job_count;
}

/* Get next job */
job_t* get_next_job() {
    for (int i = 0; i < job_count; i++) {
        if (job_queue[i].status == JOB_PENDING) {
            job_queue[i].status = JOB_RUNNING;
            save_jobs_to_file();
            return &job_queue[i];
        }
    }
    return NULL;
}

/* Update job */
void update_job(int job_id, job_status_t status, char *output) {
    for (int i = 0; i < job_count; i++) {
        if (job_queue[i].job_id == job_id) {
            job_queue[i].status = status;

            if (output) {
                strncpy(job_queue[i].output, output, MAX_OUTPUT_LEN - 1);
                job_queue[i].output[MAX_OUTPUT_LEN - 1] = '\0';
            }

            save_jobs_to_file();
            return;
        }
    }
}

/* Get job by ID */
job_t* get_job_by_id(int job_id) {
    for (int i = 0; i < job_count; i++) {
        if (job_queue[i].job_id == job_id) {
            return &job_queue[i];
        }
    }
    return NULL;
}