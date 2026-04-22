#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../include/common.h"
#include "../include/queue.h"

void execute_command(char *cmd, char *output) {
    int fd[2];
    pipe(fd);

    if (fork() == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);

        execl("/bin/sh", "sh", "-c", cmd, NULL);
        exit(1);
    } else {
        close(fd[1]);
        int n = read(fd[0], output, MAX_OUTPUT_LEN - 1);
        if (n > 0) output[n] = '\0';
        close(fd[0]);
        wait(NULL);
    }
}

int main() {
    printf("Worker started...\n");

    while (1) {
        job_t *job = get_next_job();

        if (job != NULL) {
            char output[MAX_OUTPUT_LEN] = {0};

            printf("Worker executing Job %d\n", job->job_id);

            execute_command(job->command, output);

            update_job(job->job_id, JOB_COMPLETED, output);
        }

        usleep(200000); // avoid CPU overuse
    }

    return 0;
}