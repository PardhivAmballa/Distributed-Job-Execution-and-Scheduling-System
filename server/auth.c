#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define DEFAULT_USERS_FILE "users.txt"

// Authenticates a user based on username and password
int authenticate(const char *username, const char *password, char *role) {
    const char *path = getenv("AUTH_USERS_FILE"); // Gets the path to the users file from the environment variable
    if(!path) path = DEFAULT_USERS_FILE; // If the environment variable is not set, uses the default path

    FILE *fp = fopen(path, "r");
    if(!fp){
        fprintf(stderr, "auth: cannot open users file '%s'\n", path);
        return 0;
    }

    char u[50], p[50], r[20];

    // Reads the users file and checks for a valid username and password
    while(fscanf(fp, "%49s %49s %19s", u, p, r) == 3){
        if(strcmp(u, username) == 0 && strcmp(p, password) == 0){
            if(role != NULL){
                strncpy(role, r, 19);
                role[19] = '\0';
            }
            fclose(fp);
            return 1;
        }
    }
    
    fclose(fp);
    return 0;
}