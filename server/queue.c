#include <stdio.h>
#include <string.h>
#include "../include/common.h"

job_t job_queue[MAX_JOBS];
int job_count = 0;

// Add job to queue
int add_job(char *command) {
    if (job_count >= MAX_JOBS) {
        return -1;
    }

    job_queue[job_count].job_id = job_count + 1;
    strcpy(job_queue[job_count].command, command);
    job_queue[job_count].status = JOB_PENDING;

    job_count++;
    return job_count; // return job ID
}

// Get next pending job
job_t* get_next_job() {
    for (int i = 0; i < job_count; i++) {
        if (job_queue[i].status == JOB_PENDING) {
            job_queue[i].status = JOB_RUNNING;
            return &job_queue[i];
        }
    }
    return NULL;
}

// Update job status
void update_job(int job_id, job_status_t status, char *output) {
    for (int i = 0; i < job_count; i++) {
        if (job_queue[i].job_id == job_id) {
            job_queue[i].status = status;
            if (output != NULL) {
                strcpy(job_queue[i].output, output);
            }
            return;
        }
    }
}