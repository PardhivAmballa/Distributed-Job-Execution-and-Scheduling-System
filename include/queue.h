#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"

int add_job(char *command);
job_t* get_next_job();
void update_job(int job_id, job_status_t status, char *output);

#endif