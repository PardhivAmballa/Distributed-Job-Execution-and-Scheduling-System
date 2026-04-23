#ifndef COMMON_H
#define COMMON_H

#define MAX_CMD_LEN 256
#define MAX_OUTPUT_LEN 1024
#define MAX_JOBS 128

typedef enum {
    JOB_PENDING,
    JOB_RUNNING,
    JOB_COMPLETED,
    JOB_FAILED
} job_status_t;

typedef struct {
    int job_id;
    char command[MAX_CMD_LEN];
    job_status_t status;
    char output[MAX_OUTPUT_LEN];
} job_t;

typedef struct {
    int type;
    char username[50];
    char password[50];
    job_t job;
} message_t;

#define MSG_LOGIN  1
#define MSG_SUBMIT 2
#define MSG_STATUS 3
#define MSG_RESULT 4
#define MSG_ASSIGN 5
#define MSG_COMPLETE 6

#endif