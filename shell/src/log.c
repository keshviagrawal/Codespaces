
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include "../include/log.h"
#include "../include/executor.h"
#include "../include/parser.h"
#include "../include/hop.h"
#include "../include/reveal.h"

#define MAX_HISTORY_SIZE 15
#define HISTORY_FILE_NAME ".shell_history"
#define MAX_PATH_LENGTH 1024

static char* history[MAX_HISTORY_SIZE];
static int history_count = 0;

static void get_history_file_path(char* path_buffer) {
    const char* home_dir = getenv("HOME");
    if (!home_dir) {
        // Fallback if HOME environment variable is not set.
        struct passwd *pw = getpwuid(getuid());
        if (pw) {
            home_dir = pw->pw_dir;
        } else {
            home_dir = "/"; // Last resort
        }
    }
    snprintf(path_buffer, MAX_PATH_LENGTH, "%s/%s", home_dir, HISTORY_FILE_NAME);
}

static void load_history() {
    char history_file_path[MAX_PATH_LENGTH];
    get_history_file_path(history_file_path);

    FILE* fp = fopen(history_file_path, "r");
    if (!fp) {
        return; // File doesn't exist, which is fine.
    }

    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, fp)) != -1) {
        line[strcspn(line, "\n")] = '\0';
        if (history_count < MAX_HISTORY_SIZE) {
            history[history_count++] = strdup(line);
        }
    }

    free(line);
    fclose(fp);
}


static void save_history() {
    char history_file_path[MAX_PATH_LENGTH];
    get_history_file_path(history_file_path);

    FILE* fp = fopen(history_file_path, "w");
    if (!fp) {
        perror("Failed to save history");
        return;
    }

    for (int i = 0; i < history_count; i++) {
        fprintf(fp, "%s\n", history[i]);
    }

    fclose(fp);
}


static void print_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%s\n", history[i]);
    }
}


static void purge_history() {
    for (int i = 0; i < history_count; i++) {
        free(history[i]);
        history[i] = NULL;
    }
    history_count = 0;
    save_history();
}


static void execute_from_history(int index, char** prev_dir, const char* home_dir) {
    if (index > 0 && index <= history_count) {
        int oldest_to_newest_index = history_count - index;
        char* command = history[oldest_to_newest_index];
        // printf("%s\n", command);

        char* line_copy = strdup(command);
        if (!line_copy) {
            perror("strdup failed");
            return;
        }

        char* tokens[100];
        int token_count = 0;
        tokenize_input(line_copy, tokens, &token_count);

        if (token_count > 0) {
            if (strcmp(tokens[0], "hop") == 0) {
                hop(&tokens[1], token_count - 1, prev_dir, home_dir);
            } else if (strcmp(tokens[0], "reveal") == 0) {
                reveal(&tokens[1], token_count - 1, prev_dir, home_dir);
            } else if (strcmp(tokens[0], "log") == 0) {
                fprintf(stderr, "Cannot execute 'log' command from history.\n");
            } else {
                execute_command(tokens, token_count);
            }
        }

        for (int i = 0; i < token_count; i++) {
            free(tokens[i]);
        }
        free(line_copy);

    } else {
        fprintf(stderr, "Invalid history index.\n");
    }
}


void init_log() {
    load_history();
}

void add_to_log(const char* command) {
    // Only skip the "log" command itself, but log all subcommands.
    // Use strcmp for an exact match.


    // if (strcmp(command, "log") == 0) {
    //     return;
    // }

    if (strncmp(command, "log", 3) == 0 && (command[3] == ' ' || command[3] == '\0')) {
        return;
    }
    // Do not add if identical to the last command.
    if (history_count > 0 && strcmp(history[history_count - 1], command) == 0) {
        return;
    }

    if (history_count >= MAX_HISTORY_SIZE) {
        // Overwrite the oldest command (index 0)
        free(history[0]);
        for (int i = 1; i < MAX_HISTORY_SIZE; i++) {
            history[i - 1] = history[i];
        }
        history[MAX_HISTORY_SIZE - 1] = strdup(command);
    } else {
        history[history_count++] = strdup(command);
    }

    save_history();
}

bool handle_log_command(char** args, int num_args, char** prev_dir, const char* home_dir) {
    if (num_args == 0) {
        print_history();
        return true;
    } else if (num_args == 1 && strcmp(args[0], "purge") == 0) {
        purge_history();
        return true;
    } else if (num_args == 2 && strcmp(args[0], "execute") == 0) {
        int index = atoi(args[1]);
        execute_from_history(index, prev_dir, home_dir);
        return true;
    } else {
        fprintf(stderr, "log: Invalid Syntax!\n");
        return false;
    }
}
