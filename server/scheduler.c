#include "../include/queue.h"
#include "../include/scheduler.h"

job_t *schedule_job() {
    return get_next_job();
}