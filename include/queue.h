#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"

int add_job(char *command, char *owner); // Add job to queue
job_t* get_next_job(); // Get next job from queue
job_t* get_job_by_id(int job_id); // Get job by ID
void update_job(int job_id, job_status_t status, char *output); // Update job status

void load_jobs_from_file(); // Load jobs from file
void save_jobs_to_file(); // Save jobs to file

#endif