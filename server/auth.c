#include <stdio.h>
#include <string.h>
#include <pthread.h>

/* Bug 11 fix: limit fscanf field widths to prevent stack buffer overflow */
/* Bug 10 fix: users.txt is resolved relative to project root;
   callers should ensure the working directory is the project root,
   or set an absolute path via the AUTH_USERS_FILE env variable.      */

#define DEFAULT_USERS_FILE "users.txt"

int authenticate(char *username, char *password) {
    const char *path = getenv("AUTH_USERS_FILE");
    if (!path) path = DEFAULT_USERS_FILE;

    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "auth: cannot open users file '%s'\n", path);
        return 0;
    }

    char u[50], p[50];

    /* Bug 11 fix: use width-limited format specifiers */
    while (fscanf(fp, "%49s %49s", u, p) == 2) {
        if (strcmp(u, username) == 0 && strcmp(p, password) == 0) {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}