#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/file.h>

void log_job(int job_id, char *command, char *status) {
    int fd = open("logs/system.log", O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (fd < 0) {
        perror("log open failed");
        return;
    }

    flock(fd, LOCK_EX);

    char buffer[512];
    snprintf(buffer, sizeof(buffer),
        "JobID: %d | Command: %s | Status: %s\n",
        job_id, command, status);

    write(fd, buffer, strlen(buffer));

    flock(fd, LOCK_UN);
    close(fd);
}