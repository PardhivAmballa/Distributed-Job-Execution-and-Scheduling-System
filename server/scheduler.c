#include "../include/queue.h"
#include "../include/scheduler.h"

// Schedules a job using FIFO
job_t *schedule_job(){
    return get_next_job();
}