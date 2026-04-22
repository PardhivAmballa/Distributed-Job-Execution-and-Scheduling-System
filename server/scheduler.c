#include "../include/queue.h"
#include "../include/common.h"

// Simple scheduler: fetch next job
job_t* schedule_job() {
    return get_next_job();
}