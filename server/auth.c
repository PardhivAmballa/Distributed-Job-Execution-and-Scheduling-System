#include <stdio.h>
#include <string.h>

int authenticate(char *username, char *password) {
    FILE *fp = fopen("users.txt", "r");
    if (!fp) return 0;

    char u[50], p[50];

    while (fscanf(fp, "%s %s", u, p) != EOF) {
        if (strcmp(u, username) == 0 && strcmp(p, password) == 0) {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}