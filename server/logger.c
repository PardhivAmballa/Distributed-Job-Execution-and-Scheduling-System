#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/file.h>

/* Bug 12 fix: log path is configurable via LOG_FILE env variable so the server
   can be launched from any working directory without losing log output.
   Bug 13 fix: use snprintf instead of sprintf to prevent buffer overflow. */

#define DEFAULT_LOG_FILE "logs/system.log"

void log_job(int job_id, char *command, char *status) {
    const char *path = getenv("LOG_FILE");
    if (!path) path = DEFAULT_LOG_FILE;

    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("log_job: open");
        return;
    }

    flock(fd, LOCK_EX);

    /* Bug 13 fix: snprintf caps output at sizeof(buffer) - 1 */
    char buffer[512];
    snprintf(buffer, sizeof(buffer),
             "JobID: %d | Command: %s | Status: %s\n",
             job_id, command, status);

    write(fd, buffer, strlen(buffer));

    flock(fd, LOCK_UN);
    close(fd);
}