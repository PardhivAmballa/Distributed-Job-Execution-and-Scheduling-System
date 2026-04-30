#ifndef COMMON_H
#define COMMON_H

#define MAX_CMD_LEN 256
#define MAX_OUTPUT_LEN 1024
#define MAX_JOBS 128

// Enum for job status
typedef enum {
    JOB_PENDING,
    JOB_RUNNING,
    JOB_COMPLETED,
    JOB_FAILED
} job_status_t;

// Structure for job
typedef struct {
    int job_id;
    char command[MAX_CMD_LEN]; // Command to be executed
    job_status_t status;
    char output[MAX_OUTPUT_LEN]; // Output of the job
    char owner[50];
} job_t;

// Structure for message
typedef struct {
    int type;
    char username[50];
    char password[50];
    char role[20];
    job_t job;
} message_t;

#define MSG_LOGIN  1
#define MSG_SUBMIT 2
#define MSG_STATUS 3
#define MSG_RESULT 4
#define MSG_LOGS 5

#endif