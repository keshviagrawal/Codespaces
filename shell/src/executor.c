#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include "../include/executor.h"

// Your existing global variable for foreground PID
extern pid_t foreground_pid;
extern bool is_interactive_mode;

// The struct definition is now in executor.h, so we just declare the global variables here
BackgroundJob background_jobs[256];
int background_job_count = 0;
static int next_job_number = 1;

// Function prototypes
static void execute_single_command(char** tokens, bool run_in_background);
static pid_t execute_pipe_group(char** tokens, int token_count, bool run_in_background, const char* command_name);
static void remove_background_job_index(int index);
void remove_job_by_pid(pid_t pid);
static BackgroundJob* find_job_by_number(int job_number);
static BackgroundJob* find_most_recent_job();
static void add_background_job(pid_t pid, const char* command_name);

void fg_command(char** tokens, int token_count) {
    BackgroundJob* job = NULL;
    int status;
    
    if (token_count > 2) {
        fprintf(stderr, "Syntax: fg [job_number]\n");
        return;
    }

    if (token_count == 2) {
        int job_number = atoi(tokens[1]);
        job = find_job_by_number(job_number);
    } else {
        job = find_most_recent_job();
    }

    if (job == NULL) {
        fprintf(stderr, "No such job\n");
        return;
    }
    
    printf("%s\n", job->command_name);
    
    // FIXED: Use the process group ID, not individual PID
    pid_t pgid = getpgid(job->pid);
    if (pgid == -1) {
        perror("getpgid failed");
        return;
    }
    
    // Set the foreground PID to the process group ID
    foreground_pid = pgid;
    
    // Give terminal control to the process group
    if (tcsetpgrp(STDIN_FILENO, pgid) == -1) {
        perror("tcsetpgrp failed");
        return;
    }
    
    // Resume the process if it's stopped
    if (strcmp(job->state, "Stopped") == 0) {
        if (kill(-pgid, SIGCONT) == -1) {  // Send to process group
            perror("kill failed");
            tcsetpgrp(STDIN_FILENO, getpgrp());
            return;
        }
    }

    // Wait for the foreground job to complete or stop
    pid_t result = waitpid(-pgid, &status, WUNTRACED);  // Wait for process group
    tcsetpgrp(STDIN_FILENO, getpgrp()); // Regain terminal control
    
    if (result == -1) {
        perror("waitpid failed");
    } else if (WIFSTOPPED(status)) {
        printf("\n[%d] Stopped %s\n", job->job_number, job->command_name);
        strcpy(job->state, "Stopped");
        foreground_pid = -1;
    } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
        remove_job_by_pid(job->pid);
        foreground_pid = -1;
    }
}

void bg_command(char** tokens, int token_count) {
    BackgroundJob* job = NULL;

    if (token_count > 2) {
        fprintf(stderr, "Syntax: bg [job_number]\n");
        return;
    }

    if (token_count == 2) {
        int job_number = atoi(tokens[1]);
        job = find_job_by_number(job_number);
    } else {
        job = find_most_recent_job();
    }

    if (job == NULL) {
        fprintf(stderr, "No such job\n");
        return;
    }

    if (strcmp(job->state, "Running") == 0) {
        fprintf(stderr, "Job already running\n");
        return;
    }
    
    if (kill(job->pid, SIGCONT) == -1) {
        perror("kill failed");
        return;
    }

    strcpy(job->state, "Running");
    printf("[%d] %s &\n", job->job_number, job->command_name);
}

static void add_background_job(pid_t pid, const char* command_name) {
    if (background_job_count >= (int)(sizeof(background_jobs) / sizeof(background_jobs[0]))) {
        return;
    }
    BackgroundJob* job = &background_jobs[background_job_count++];
    job->job_number = next_job_number++;
    job->pid = pid;
    strcpy(job->state, "Running"); 
    if (command_name != NULL) {
        strncpy(job->command_name, command_name, sizeof(job->command_name) - 1);
        job->command_name[sizeof(job->command_name) - 1] = '\0';
    } else {
        job->command_name[0] = '\0';
    }

    if (is_interactive_mode) {
        fprintf(stderr, "[%d] %d\n", job->job_number, (int)job->pid);
        fflush(stderr);
    }
}

static void remove_background_job_index(int index) {
    if (index < 0 || index >= background_job_count) {
        return;
    }
    for (int i = index; i < background_job_count - 1; i++) {
        background_jobs[i] = background_jobs[i + 1];
    }
    background_job_count--;
}

void remove_job_by_pid(pid_t pid) {
    for (int i = 0; i < background_job_count; i++) {
        if (background_jobs[i].pid == pid) {
            remove_background_job_index(i);
            return;
        }
    }
}

static BackgroundJob* find_job_by_number(int job_number) {
    if (job_number <= 0) {
        return NULL;
    }
    for (int i = 0; i < background_job_count; i++) {
        if (background_jobs[i].job_number == job_number) {
            return &background_jobs[i];
        }
    }
    return NULL;
}

static BackgroundJob* find_most_recent_job() {
    if (background_job_count > 0) {
        return &background_jobs[background_job_count - 1];
    }
    return NULL;
}

void check_background_jobs(void) {
    int status;
    for (int i = 0; i < background_job_count; ) {
        pid_t pid = background_jobs[i].pid;
        pid_t result = waitpid(pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
        
        if (result == 0) {
            i++;
            continue;
        } else if (result == -1) {
            remove_background_job_index(i);
            continue;
        } else {
            const char* name = background_jobs[i].command_name[0] != '\0' ? background_jobs[i].command_name : "";
            
            if (WIFEXITED(status)) {
                if (is_interactive_mode) {
                    fprintf(stderr, "%s with pid %d exited normally\n", name, (int)pid);
                    fflush(stderr);
                }
                remove_background_job_index(i);
            } else if (WIFSIGNALED(status)) {
                if (is_interactive_mode) {
                    fprintf(stderr, "%s with pid %d exited abnormally\n", name, (int)pid);
                    fflush(stderr);
                }
                remove_background_job_index(i);
            } else if (WIFSTOPPED(status)) {
                strcpy(background_jobs[i].state, "Stopped");
                i++;
            } else if (WIFCONTINUED(status)) {
                strcpy(background_jobs[i].state, "Running");
                i++;
            }
        }
    }
}

static int compare_background_jobs(const void* a, const void* b) {
    const BackgroundJob* job_a = (const BackgroundJob*)a;
    const BackgroundJob* job_b = (const BackgroundJob*)b;
    return strcmp(job_a->command_name, job_b->command_name);
}

void list_activities(void) {
    check_background_jobs();

    BackgroundJob active_jobs[256];
    int active_jobs_count = 0;

    for(int i = 0; i < background_job_count; i++) {
        active_jobs[active_jobs_count++] = background_jobs[i];
    }

    qsort(active_jobs, active_jobs_count, sizeof(BackgroundJob), compare_background_jobs);

    for (int i = 0; i < active_jobs_count; i++) {
        printf("[%d] : %s - %s\n", active_jobs[i].pid, active_jobs[i].command_name, active_jobs[i].state);
    }
}

void ping(pid_t pid, int signal_number) {
    // Fix modulo for negative numbers
    int actual_signal = ((signal_number % 32) + 32) % 32;

    if (kill(pid, actual_signal) == -1) {
        if (errno == ESRCH) {
            fprintf(stderr, "No such process found\n");
        } else {
            perror("kill failed");
        }
    } else {
        printf("Sent signal %d to process with pid %d\n", signal_number, (int)pid);
    }
}

void check_and_kill_all_jobs(void) {
    for (int i = 0; i < background_job_count; i++) {
        kill(background_jobs[i].pid, SIGKILL);
    }
    int status;
    while (waitpid(-1, &status, 0) > 0);
}

static void execute_single_command(char** tokens, bool run_in_background) {
    int saved_stdin = dup(STDIN_FILENO);
    int saved_stdout = dup(STDOUT_FILENO);

    int in_fd = -1;
    int out_fd = -1;
    char** cmd_args = tokens;

    for (int i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "<") == 0) {
            if (tokens[i+1] == NULL) {
                fprintf(stderr, "syntax error near unexpected token `<'\n");
                exit(1);
            }
            in_fd = open(tokens[i+1], O_RDONLY);
            if (in_fd == -1) {
                fprintf(stderr, "No such file or directory!\n");
                exit(1);
            }
            tokens[i] = NULL;
        } else if (strcmp(tokens[i], ">") == 0) {
            if (tokens[i+1] == NULL) {
                fprintf(stderr, "syntax error near unexpected token `>'\n");
                exit(1);
            }
            out_fd = open(tokens[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (out_fd == -1) {
                perror(tokens[i+1]);
                exit(1);
            }
            tokens[i] = NULL;
        } else if (strcmp(tokens[i], ">>") == 0) {
            if (tokens[i+1] == NULL) {
                fprintf(stderr, "syntax error near unexpected token `>>'\n");
                exit(1);
            }
            out_fd = open(tokens[i+1], O_WRONLY | O_CREAT | O_APPEND, 0666);
            if (out_fd == -1) {
                perror(tokens[i+1]);
                exit(1);
            }
            tokens[i] = NULL;
        }
    }

    if (in_fd != -1) {
        dup2(in_fd, STDIN_FILENO);
        close(in_fd);
    } else if (run_in_background) {
        int devnull = open("/dev/null", O_RDONLY);
        if (devnull != -1) {
            dup2(devnull, STDIN_FILENO);
            close(devnull);
        }
    }
    if (out_fd != -1) {
        dup2(out_fd, STDOUT_FILENO);
        close(out_fd);
    }

    execvp(cmd_args[0], cmd_args);

    // fprintf(stderr, "%s: Command not found\n", cmd_args[0]);
    fprintf(stderr, "Command not found!\n");

    if (in_fd != -1) {
        dup2(saved_stdin, STDIN_FILENO);
    }
    if (out_fd != -1) {
        dup2(saved_stdout, STDOUT_FILENO);
    }
    close(saved_stdin);
    close(saved_stdout);

    exit(1);
}

static pid_t execute_pipe_group(char** tokens, int token_count, bool run_in_background, const char* command_name) {
    if (token_count <= 0) {
        return -1;
    }

    int pipe_count = 0;
    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], "|") == 0) {
            pipe_count++;
        }
    }

    if (pipe_count == 0) {
        // No pipes, simple command
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return -1;
        } else if (pid == 0) { // Child
            setpgid(0, 0);
            if (!run_in_background && is_interactive_mode) {
                tcsetpgrp(STDIN_FILENO, getpgrp());
            }
            execute_single_command(tokens, run_in_background);
            exit(127); // Should not be reached
        } else { // Parent
            setpgid(pid, pid);
            if (run_in_background) {
                add_background_job(pid, command_name);
                return pid;
            } else {
                foreground_pid = pid;
                tcsetpgrp(STDIN_FILENO, pid);
                
                int status;
                waitpid(pid, &status, WUNTRACED);

                if (WIFSTOPPED(status)) {
                    add_background_job(pid, command_name);
                    fprintf(stderr, "\n[%d] Stopped %s\n", background_jobs[background_job_count-1].job_number, command_name);
                }

                tcsetpgrp(STDIN_FILENO, getpgrp());
                foreground_pid = -1;
                return pid;
            }
        }
    }

    // Pipe handling logic
    int start = 0;
    pid_t pgid = 0;
    int num_cmds = pipe_count + 1;
    pid_t pids[num_cmds];
    int pid_count = 0;
    int fds[2];
    int in_fd = -1;

    for (int i = 0; i < num_cmds; i++) {
        int end = start;
        while(end < token_count && strcmp(tokens[end], "|") != 0) {
            end++;
        }
        tokens[end] = NULL;

        bool is_last = (i == num_cmds - 1);
        if (!is_last) {
            if (pipe(fds) == -1) {
                perror("pipe");
                return -1;
            }
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return -1;
        } else if (pid == 0) { // Child
            if (pgid == 0) {
                setpgid(0, 0);
            } else {
                setpgid(0, pgid);
            }

            if (in_fd != -1) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (!is_last) {
                dup2(fds[1], STDOUT_FILENO);
                close(fds[0]);
                close(fds[1]);
            }

            execute_single_command(&tokens[start], run_in_background);
            exit(127);
        } else { // Parent
            if (pgid == 0) {
                pgid = pid;
            }
            setpgid(pid, pgid);
            pids[pid_count++] = pid;

            if (in_fd != -1) {
                close(in_fd);
            }
            if (!is_last) {
                close(fds[1]);
                in_fd = fds[0];
            }
            start = end + 1;
        }
    }

    if (run_in_background) {
        for (int i = 0; i < pid_count; i++) {
            add_background_job(pids[i], command_name);
        }
        return pgid;
    }
    
    foreground_pid = pgid;
    if (pgid != 0) {
        tcsetpgrp(STDIN_FILENO, pgid);
    }
    
    int status;
    for (int i = 0; i < pid_count; i++) {
        waitpid(pids[i], &status, WUNTRACED);
        if (WIFSTOPPED(status)) {
            add_background_job(pids[i], command_name);
            if (i == pid_count - 1) {
                fprintf(stderr, "\n[%d] Stopped %s\n", background_jobs[background_job_count-1].job_number, command_name);
            }
        }
    }
    
    tcsetpgrp(STDIN_FILENO, getpgrp());
    foreground_pid = -1;

    return pgid;
}

bool execute_command(char** tokens, int token_count) {
    if (token_count <= 0) {
        return false;
    }

    int cmd_start = 0;
    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], ";") == 0 || strcmp(tokens[i], "&") == 0) {
            bool background = (strcmp(tokens[i], "&") == 0);
            tokens[i] = NULL;
            const char* command_name = (cmd_start < token_count && tokens[cmd_start] != NULL) ? tokens[cmd_start] : "";
            if (execute_pipe_group(&tokens[cmd_start], i - cmd_start, background, command_name) == -1) {
                return false;
            }
            cmd_start = i + 1;
        }
    }

    if (cmd_start < token_count) {
        const char* command_name = (tokens[cmd_start] != NULL) ? tokens[cmd_start] : "";
        if (execute_pipe_group(&tokens[cmd_start], token_count - cmd_start, false, command_name) == -1) {
            return false;
        }
    }

    return true;
}