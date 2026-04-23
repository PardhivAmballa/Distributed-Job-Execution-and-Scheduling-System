#include "../include/queue.h"
#include "../include/scheduler.h"

/*
 * Bug 14 fix: server.c now calls schedule_job() instead of get_next_job()
 * directly, so the scheduler layer is properly used. Swap the body of this
 * function to implement a different scheduling policy (e.g. priority, round-
 * robin) without touching server.c or queue.c.
 */
job_t *schedule_job() {
    return get_next_job();
}