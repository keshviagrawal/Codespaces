#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/limits.h>
#include "../include/hop.h"

// A utility function to change directory and update the previous directory.
bool change_directory(const char* path, char** prev_dir) {
    char old_dir[PATH_MAX];
    if (getcwd(old_dir, sizeof(old_dir)) == NULL) {
        perror("getcwd failed");
        return false;
    }

    if (chdir(path) != 0) {
        perror("chdir failed");
        return false;
    }

    if (*prev_dir != NULL) {
        free(*prev_dir);
    }
    *prev_dir = strdup(old_dir);
    return true;
}

bool hop(char** args, int num_args, char** prev_dir, const char* home_dir) {
    char target_path[PATH_MAX];

    if (num_args == 0) {
        // No arguments, go to home directory
        strncpy(target_path, home_dir, PATH_MAX - 1);
        target_path[PATH_MAX - 1] = '\0';
    } else if (num_args == 1) {
        // One argument
        if (strcmp(args[0], "-") == 0) {
            // Go to previous directory
            if (*prev_dir == NULL) {
                fprintf(stderr, "shell: hop: prev_dir not set\n");
                return false;
            }
            strncpy(target_path, *prev_dir, PATH_MAX - 1);
            target_path[PATH_MAX - 1] = '\0';
        } else if (strcmp(args[0], "~") == 0) {
            // Go to home directory
            strncpy(target_path, home_dir, PATH_MAX - 1);
            target_path[PATH_MAX - 1] = '\0';
        } else {
            // Go to specified path
            strncpy(target_path, args[0], PATH_MAX - 1);
            target_path[PATH_MAX - 1] = '\0';
        }
    } else {
        // More than one argument
        fprintf(stderr, "hop: too many arguments\n");
        return false;
    }

    if (change_directory(target_path, prev_dir)) {
        if (num_args == 1 && strcmp(args[0], "-") == 0) {
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            }
        }
        return true;
    }

    return false;
}