#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/common.h"

/*
 * NOTE: The caller (server.c) is responsible for holding queue_mutex
 * around all calls into this module to ensure thread safety (Bug 8 fix).
 */

job_t job_queue[MAX_JOBS];
int job_count = 0;

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
    return job_count; /* job_id == job_count after increment */
}

/*
 * Get the next pending job and mark it RUNNING.
 *
 * Bug 9 fix: The caller should handle the case where the worker that received
 * this job disconnects before completing it. A production system would use a
 * heartbeat / timeout to revert JOB_RUNNING back to JOB_PENDING. That logic
 * lives in the scheduler; here we at least document the invariant clearly.
 */
job_t *get_next_job() {
    for (int i = 0; i < job_count; i++) {
        if (job_queue[i].status == JOB_PENDING) {
            job_queue[i].status = JOB_RUNNING;
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