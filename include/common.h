#ifndef COMMON_H
#define COMMON_H

#define MAX_CMD_LEN 256
#define MAX_OUTPUT_LEN 1024
#define MAX_JOBS 100

// Job Status
typedef enum {
    JOB_PENDING,
    JOB_RUNNING,
    JOB_COMPLETED,
    JOB_FAILED
} job_status_t;


// Job Structure
typedef struct {
    int job_id;
    char command[MAX_CMD_LEN];
    job_status_t status;
    char output[MAX_OUTPUT_LEN];
} job_t;


// Message between client and server
typedef struct {
    int type; // 1 = submit job, 2 = get result
    job_t job;
} message_t;

#endif