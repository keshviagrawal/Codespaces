#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "../include/hop.h"

/**
 * @brief Prints the current working directory to the console.
 */

// void print_cwd() {
//     char cwd[1024];
//     if (getcwd(cwd, sizeof(cwd)) != NULL) {
//         printf("<rudy@iiit:%s> ", cwd);
//     } else {
//         perror("getcwd");
//     }
// }

bool change_directory(const char* path, char** prev_dir) {
    char old_dir[1024];
    if (getcwd(old_dir, sizeof(old_dir)) == NULL) {
        perror("getcwd failed before changing directory");
        return false;
    }

    if (chdir(path) != 0) {
        printf("No such directory!\n");
        return false;
    }

    if (*prev_dir != NULL) {
        free(*prev_dir);
    }
    *prev_dir = strdup(old_dir);

    return true;
}

bool hop(char** args, int num_args, char** prev_dir, const char* home_dir) {
    // If no arguments, or the first argument is "~", change to home directory.
    if (num_args == 0 || (num_args > 0 && strcmp(args[0], "~") == 0)) {
        if (home_dir == NULL) {
            fprintf(stderr, "Home directory not set.\n");
            return false;
        }
        if (!change_directory(home_dir, prev_dir)) {
            return false;
        }
        if (num_args == 0) return true; // Exit if no args were given
    }
    
    // Process remaining arguments sequentially
    int start_index = (num_args > 0 && strcmp(args[0], "~") == 0) ? 1 : 0;
    
    for (int i = start_index; i < num_args; i++) {
        char* arg = args[i];
        
        if (strcmp(arg, ".") == 0) {
            // Do nothing
            continue;
        } else if (strcmp(arg, "..") == 0) {
            if (!change_directory("..", prev_dir)) {
                return false;
            }
        } else if (strcmp(arg, "-") == 0) {
            if (*prev_dir == NULL) {
                fprintf(stderr, "No previous directory.\n");
                return false;
            }
            char* temp_prev = strdup(*prev_dir);
            if (!change_directory(temp_prev, prev_dir)) {
                free(temp_prev);
                return false;
            }
            free(temp_prev);
        } else if (strcmp(arg, "~") == 0) {
            if (!change_directory(home_dir, prev_dir)) {
                return false;
            }
        } else {
            // Assume it's a directory name
            if (!change_directory(arg, prev_dir)) {
                return false;
            }
        }
    }

    return true;
}
