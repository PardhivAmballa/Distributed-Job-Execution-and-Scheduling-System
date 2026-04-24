#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"

int add_job(char *command);
job_t* get_next_job();
job_t* get_job_by_id(int job_id);
void update_job(int job_id, job_status_t status, char *output);

void load_jobs_from_file();
void save_jobs_to_file();

#endif