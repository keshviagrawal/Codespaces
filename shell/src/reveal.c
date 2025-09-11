#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <strings.h>
#include "../include/reveal.h"

// Comparison function for qsort to sort strings lexicographically (case-insensitive).
int compare_strings(const void* a, const void* b) {
    const char* str_a = *(const char**)a;
    const char* str_b = *(const char**)b;
    return strcasecmp(str_a, str_b);
}

bool reveal(char** args, int num_args, char** prev_dir, const char* home_dir) {
    // Suppress unused parameter warnings
    (void)prev_dir;
    (void)home_dir;

    bool show_all = false;
    bool line_by_line = false;
    const char* path_arg = NULL;

    // Parse flags and identify the path argument
    for (int i = 0; i < num_args; i++) {
        if (strcmp(args[i], "-al") == 0 || strcmp(args[i], "-la") == 0) {
            show_all = true;
            line_by_line = true;
        } else if (strcmp(args[i], "-a") == 0) {
            show_all = true;
        } else if (strcmp(args[i], "-l") == 0) {
            line_by_line = true;
        } else {
            if (path_arg != NULL) {
                fprintf(stderr, "reveal: too many arguments\n");
                return false;
            }
            path_arg = args[i];
        }
    }

    char target_path[PATH_MAX];
    if (path_arg == NULL) {
        // No path argument given, default to the current directory
        if (getcwd(target_path, sizeof(target_path)) == NULL) {
            perror("getcwd failed");
            return false;
        }
    } else {
        // Path argument is provided (and should be expanded by main)
        strncpy(target_path, path_arg, sizeof(target_path) - 1);
        target_path[sizeof(target_path) - 1] = '\0';
    }

    DIR* dir = opendir(target_path);
    if (dir == NULL) {
        perror("reveal");
        return false;
    }

    struct dirent* entry;
    char** filenames = NULL;
    int file_count = 0;
    int max_files = 10;

    filenames = (char**)malloc(max_files * sizeof(char*));
    if (filenames == NULL) {
        perror("malloc failed");
        closedir(dir);
        return false;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (!show_all && entry->d_name[0] == '.') {
            continue;
        }

        if (file_count >= max_files) {
            max_files *= 2;
            char** new_filenames = (char**)realloc(filenames, max_files * sizeof(char*));
            if (new_filenames == NULL) {
                perror("realloc failed");
                // Free existing memory before returning
                for (int i = 0; i < file_count; i++) free(filenames[i]);
                free(filenames);
                closedir(dir);
                return false;
            }
            filenames = new_filenames;
        }
        filenames[file_count++] = strdup(entry->d_name);
    }
    closedir(dir);

    qsort(filenames, file_count, sizeof(char*), compare_strings);

    for (int i = 0; i < file_count; i++) {
        printf("%s", filenames[i]);
        if (line_by_line) {
            printf("\n");
        } else {
            printf("  ");
        }
        free(filenames[i]);
    }
    if (!line_by_line && file_count > 0) {
        printf("\n");
    }

    free(filenames);
    return true;
}
