#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdbool.h>   // ✅ for bool type
#include "../include/input_redir.h"

// Strip quotes helper
static void strip_quotes(char* arg) {
    int len = strlen(arg);
    if (len >= 2 && arg[0] == '"' && arg[len - 1] == '"') {
        memmove(arg, arg + 1, len - 2);
        arg[len - 2] = '\0';
    }
}

// Execute single command (atomic) with redirection & pipes
static void execute_atomic_command(char** tokens, bool is_first, bool is_last, int pipe_in[2], int pipe_out[2]) {
    char* input_file = NULL;
    char* output_file = NULL;
    int output_append = 0;

    int token_count = 0;
    for (; tokens[token_count] != NULL; token_count++);

    // Detect redirections and remove from tokens
    for (int i = 0; i < token_count; i++) {
        if (tokens[i] == NULL) continue;

        if (strcmp(tokens[i], "<") == 0 && i + 1 < token_count) {
            input_file = tokens[i + 1];
            tokens[i] = tokens[i + 1] = NULL;
            i++;
        } else if ((strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], ">>") == 0) && i + 1 < token_count) {
            output_file = tokens[i + 1];
            output_append = (strcmp(tokens[i], ">>") == 0) ? 1 : 0;
            tokens[i] = tokens[i + 1] = NULL;
            i++;
        }
    }

    // Build argv
    char* argv[token_count + 1];
    int argc = 0;
    for (int i = 0; i < token_count; i++) {
        if (tokens[i] != NULL) {
            strip_quotes(tokens[i]);
            argv[argc++] = tokens[i];
        }
    }
    argv[argc] = NULL;

    if (argc == 0) {
        fprintf(stderr, "syntax error: missing command\n");
        exit(EXIT_FAILURE);
    }

    // ✅ Setup pipes
    if (!is_first) dup2(pipe_in[0], STDIN_FILENO);
    if (!is_last) dup2(pipe_out[1], STDOUT_FILENO);

    // ✅ Input redirection overrides pipe
    if (input_file) {
        int fd = open(input_file, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "No such file or directory!\n");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    // ✅ Output redirection overrides pipe
    if (output_file) {
        int fd = (output_append)
                    ? open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644)
                    : open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) { perror("open failed"); exit(EXIT_FAILURE); }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    // ✅ Close all pipes in child
    if (pipe_in[0] != -1) close(pipe_in[0]);
    if (pipe_in[1] != -1) close(pipe_in[1]);
    if (pipe_out[0] != -1) close(pipe_out[0]);
    if (pipe_out[1] != -1) close(pipe_out[1]);

    execvp(argv[0], argv);

    if (errno == ENOENT)
        fprintf(stderr, "%s: command not found\n", argv[0]);
    else
        perror("execvp failed");
    exit(EXIT_FAILURE);
}

// Execute a group of commands (handles pipes)
bool execute_command_group(char** tokens, int token_count) {
    if (token_count == 0) return false;
    if (strcmp(tokens[0], "exit") == 0) exit(0);

    int start = 0;
    int prev_pipe[2] = {-1, -1};
    pid_t pids[token_count];
    int pid_count = 0;

    for (int i = 0; i <= token_count; i++) {
        if (i == token_count || strcmp(tokens[i], "|") == 0) {
            int end = i - 1;
            char* cmd_tokens[token_count + 1];
            int cmd_len = 0;
            for (int j = start; j <= end; j++) cmd_tokens[cmd_len++] = tokens[j];
            cmd_tokens[cmd_len] = NULL;

            int cur_pipe[2] = {-1, -1};
            bool is_last = (i == token_count);
            if (!is_last && pipe(cur_pipe) == -1) { perror("pipe failed"); return false; }

            pid_t pid = fork();
            if (pid == -1) { perror("fork failed"); return false; }
            else if (pid == 0) {
                execute_atomic_command(cmd_tokens, start == 0, is_last, prev_pipe, cur_pipe);
            } else {
                pids[pid_count++] = pid;

                // ✅ Parent closes unused pipe ends immediately
                if (prev_pipe[0] != -1) { close(prev_pipe[0]); close(prev_pipe[1]); }
                if (!is_last) { prev_pipe[0] = cur_pipe[0]; prev_pipe[1] = cur_pipe[1]; }
                else { prev_pipe[0] = prev_pipe[1] = -1; }
            }

            start = i + 1;
        }
    }

    // ✅ Wait for all children
    for (int i = 0; i < pid_count; i++) waitpid(pids[i], NULL, 0);
    return true;
}

