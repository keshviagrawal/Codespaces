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
#include "../include/hop.h"
#include "../include/reveal.h"
#include "../include/log.h"

// Global variables for job control
extern pid_t foreground_pid;
extern bool is_interactive_mode;

BackgroundJob background_jobs[256];
int background_job_count = 0;
static int next_job_number = 1;

// Forward declarations
static void run_command_in_child(char** tokens, int token_count, bool run_in_background, char** prev_dir, char* SHELL_HOME_DIR);
static pid_t execute_pipeline(char** tokens, int token_count, bool run_in_background, const char* command_name, char** prev_dir, char* SHELL_HOME_DIR);
static void execute_builtin(char** tokens, int token_count, char** prev_dir, char* SHELL_HOME_DIR);

// Helper to get job state as a string
static const char* get_job_state_string(JobState state) {
    switch (state) {
        case RUNNING:
            return "Running";
        case STOPPED:
            return "Stopped";
        default:
            return "Unknown";
    }
}

// Job Management Functions
static void add_background_job(pid_t pid, const char* command_name, JobState state) {
    if (background_job_count >= (int)(sizeof(background_jobs) / sizeof(background_jobs[0]))) {
        return;
    }
    BackgroundJob* job = &background_jobs[background_job_count++];
    job->job_number = next_job_number++;
    job->pid = pid;
    job->state = state;
    if (command_name != NULL) {
        strncpy(job->command_name, command_name, sizeof(job->command_name) - 1);
        job->command_name[sizeof(job->command_name) - 1] = '\0';
    } else {
        job->command_name[0] = '\0';
    }

    if (is_interactive_mode && state == RUNNING) {
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
    if (job_number <= 0) return NULL;
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
        pid_t result = waitpid(-pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
        
        if (result == 0) {
            i++;
            continue;
        } else if (result == -1) {
            if (errno == ECHILD) { // No more processes in the process group
                remove_background_job_index(i);
            } else {
                i++;
            }
            continue;
        } else {
            const char* name = background_jobs[i].command_name[0] != '\0' ? background_jobs[i].command_name : "";
            
            if (WIFEXITED(status)) {
                if (is_interactive_mode) {
                    if (WEXITSTATUS(status) != 0)
                        fprintf(stderr, "%s with pid %d exited abnormally\n", name, (int)pid, WEXITSTATUS(status));
                    else fprintf(stderr, "%s with pid %d exited normally\n", name, (int)pid);
                    fflush(stderr);
                }
                remove_background_job_index(i);
            } else if (WIFSTOPPED(status)) {
                background_jobs[i].state = STOPPED;
                i++;
            } else if (WIFCONTINUED(status)) {
                background_jobs[i].state = RUNNING;
                i++;
            }
        }
    }
}

static int compare_background_jobs(const void* a, const void* b) {
    return strcmp(((const BackgroundJob*)a)->command_name, ((const BackgroundJob*)b)->command_name);
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
        printf("[%d] : %s - %s\n", active_jobs[i].pid, active_jobs[i].command_name, get_job_state_string(active_jobs[i].state));
    }
}

void ping(pid_t pid, int signal_number) {
    int actual_signal = ((signal_number % 32) + 32) % 32;
    if (kill(pid, actual_signal) == -1) {
        if (errno == ESRCH) fprintf(stderr, "No such process found\n");
        else perror("kill failed");
    } else {
        printf("Sent signal %d to process with pid %d\n", signal_number, (int)pid);
    }
}

void check_and_kill_all_jobs(void) {
    for (int i = 0; i < background_job_count; i++) {
        kill(-background_jobs[i].pid, SIGKILL);
    }
    while (waitpid(-1, NULL, 0) > 0);
}

void fg_command(char** tokens, int token_count) {
    BackgroundJob* job = NULL;
    if (token_count > 2) {
        fprintf(stderr, "Syntax: fg [job_number]\n");
        return;
    }
    job = (token_count == 2) ? find_job_by_number(atoi(tokens[1])) : find_most_recent_job();
    if (job == NULL) {
        fprintf(stderr, "No such job\n");
        return;
    }
    
    printf("%s\n", job->command_name);
    pid_t pgid = job->pid;
    
    foreground_pid = pgid;
    if (tcsetpgrp(STDIN_FILENO, pgid) == -1) {
        perror("tcsetpgrp failed");
        foreground_pid = -1;
        return;
    }
    
    if (job->state == STOPPED) {
        if (kill(-pgid, SIGCONT) == -1) {
            perror("kill failed");
            tcsetpgrp(STDIN_FILENO, getpgrp());
            foreground_pid = -1;
            return;
        }
    }

    int status;
    if (waitpid(-pgid, &status, WUNTRACED) != -1) {
        if (WIFSTOPPED(status)) {
            fprintf(stderr, "\n[%d] Stopped %s\n", job->job_number, job->command_name);
            job->state = STOPPED;
        } else {
            remove_job_by_pid(job->pid);
        }
    } else if (errno == ECHILD) {
        remove_job_by_pid(job->pid);
    }

    tcsetpgrp(STDIN_FILENO, getpgrp());
    foreground_pid = -1;
}

void bg_command(char** tokens, int token_count) {
    BackgroundJob* job = NULL;
    if (token_count > 2) {
        fprintf(stderr, "Syntax: bg [job_number]\n");
        return;
    }
    job = (token_count == 2) ? find_job_by_number(atoi(tokens[1])) : find_most_recent_job();
    if (job == NULL) {
        fprintf(stderr, "No such job\n");
        return;
    }
    if (job->state == RUNNING) {
        fprintf(stderr, "Job already running\n");
        return;
    }
    if (kill(-job->pid, SIGCONT) == -1) {
        perror("kill failed");
        return;
    }
    job->state = RUNNING;
    printf("[%d] %s &\n", job->job_number, job->command_name);
}

// --- New Execution Logic ---

enum BuiltinType { NOT_BUILTIN, SPECIAL_BUILTIN, REGULAR_BUILTIN };

static enum BuiltinType get_builtin_type(const char* cmd) {
    if (!cmd) return NOT_BUILTIN;
    if (strcmp(cmd, "hop") == 0 || strcmp(cmd, "exit") == 0 || strcmp(cmd, "fg") == 0 || strcmp(cmd, "bg") == 0) {
        return SPECIAL_BUILTIN;
    }
    if (strcmp(cmd, "reveal") == 0 || strcmp(cmd, "log") == 0 || strcmp(cmd, "activities") == 0 || strcmp(cmd, "ping") == 0) {
        return REGULAR_BUILTIN;
    }
    return NOT_BUILTIN;
}

static void execute_builtin(char** tokens, int token_count, char** prev_dir, char* SHELL_HOME_DIR) {
    if (strcmp(tokens[0], "hop") == 0) {
        hop(&tokens[1], token_count - 1, prev_dir, SHELL_HOME_DIR);
    } else if (strcmp(tokens[0], "exit") == 0) {
        check_and_kill_all_jobs();
        printf("logout\n");
        exit(0);
    } else if (strcmp(tokens[0], "fg") == 0) {
        fg_command(tokens, token_count);
    } else if (strcmp(tokens[0], "bg") == 0) {
        bg_command(tokens, token_count);
    } else if (strcmp(tokens[0], "reveal") == 0) {
        reveal(&tokens[1], token_count - 1, prev_dir, SHELL_HOME_DIR);
    } else if (strcmp(tokens[0], "log") == 0) {
        handle_log_command(&tokens[1], token_count - 1, prev_dir, SHELL_HOME_DIR);
    } else if (strcmp(tokens[0], "activities") == 0) {
        list_activities();
    } else if (strcmp(tokens[0], "ping") == 0) {
        if (token_count != 3) fprintf(stderr, "Syntax: ping <pid> <signal_number>\n");
        else ping((pid_t)strtol(tokens[1], NULL, 10), (int)strtol(tokens[2], NULL, 10));
    }
}

static void run_command_in_child(char** tokens, int token_count, bool run_in_background, char** prev_dir, char* SHELL_HOME_DIR) {
    char* cmd_args[1024];
    int arg_count = 0;
    int in_fd = -1, out_fd = -1;

    for (int i = 0; i < token_count && tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "<") == 0) {
            if (!tokens[++i]) { fprintf(stderr, "Syntax error near `<`\n"); exit(1); }
            if ((in_fd = open(tokens[i], O_RDONLY)) == -1) { printf("No such file or directory\n"); exit(1); }
        } else if (strcmp(tokens[i], ">") == 0) {
            if (!tokens[++i]) { fprintf(stderr, "Syntax error near `>`\n"); exit(1); }
            if ((out_fd = open(tokens[i], O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) { printf("Unable to create file for writing\n"); exit(1); }
        } else if (strcmp(tokens[i], ">>") == 0) {
            if (!tokens[++i]) { fprintf(stderr, "Syntax error near `>>`\n"); exit(1); }
            if ((out_fd = open(tokens[i], O_WRONLY | O_CREAT | O_APPEND, 0666)) == -1) { printf("Unable to create file for writing\n"); exit(1); }
        } else {
            cmd_args[arg_count++] = tokens[i];
        }
    }
    cmd_args[arg_count] = NULL;

    if (in_fd != -1) { dup2(in_fd, STDIN_FILENO); close(in_fd); }
    if (out_fd != -1) { dup2(out_fd, STDOUT_FILENO); close(out_fd); }
    if (run_in_background && in_fd == -1) {
        int devnull = open("/dev/null", O_RDONLY);
        if (devnull != -1) { dup2(devnull, STDIN_FILENO); close(devnull); }
    }

    if (arg_count > 0 && get_builtin_type(cmd_args[0]) != NOT_BUILTIN) {
        if (strcmp(cmd_args[0], "fg") == 0 || strcmp(cmd_args[0], "bg") == 0) {
            fprintf(stderr, "%s: no job control\n", cmd_args[0]);
            exit(1);
        }
        execute_builtin(cmd_args, arg_count, prev_dir, SHELL_HOME_DIR);
        exit(0);
    }
    
    if (arg_count > 0) {
        execvp(cmd_args[0], cmd_args);
        fprintf(stderr, "%s: Command not found!\n", cmd_args[0]);
    }
    exit(127);
}

static pid_t execute_pipeline(char** tokens, int token_count, bool run_in_background, const char* command_name, char** prev_dir, char* SHELL_HOME_DIR) {
    if (token_count <= 0) return -1;

    int pipe_count = 0;
    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], "|") == 0) pipe_count++;
    }

    if (pipe_count == 0) {
        char* cmd_args[1024];
        int arg_count = 0;
        bool has_redirection = false;
        for (int i = 0; i < token_count; i++) {
            if (strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], ">>") == 0) {
                has_redirection = true;
                i++;
            } else {
                cmd_args[arg_count++] = tokens[i];
            }
        }
        cmd_args[arg_count] = NULL;

        if (arg_count > 0 && get_builtin_type(cmd_args[0]) == SPECIAL_BUILTIN) {
            if (has_redirection) {
                fprintf(stderr, "shell: redirection is not supported for %s\n", cmd_args[0]);
                return 0;
            }
            execute_builtin(cmd_args, arg_count, prev_dir, SHELL_HOME_DIR);
            return 0;
        }

        pid_t pid = fork();
        if (pid == -1) { perror("fork"); return -1; }
        if (pid == 0) {
            if (is_interactive_mode) {
                setpgid(0, 0);
                if (!run_in_background) tcsetpgrp(STDIN_FILENO, getpgrp());
            }
            run_command_in_child(tokens, token_count, run_in_background, prev_dir, SHELL_HOME_DIR);
        }

        setpgid(pid, pid);
        if (run_in_background) {
            add_background_job(pid, command_name, RUNNING);
            return pid;
        }
        
        foreground_pid = pid;
        if (is_interactive_mode) tcsetpgrp(STDIN_FILENO, pid);
        
        int status;
        if (waitpid(pid, &status, WUNTRACED) != -1) {
            if (WIFSTOPPED(status)) {
                add_background_job(pid, command_name, STOPPED);
                fprintf(stderr, "\n[%d] Stopped %s\n", background_jobs[background_job_count-1].job_number, command_name);
            }
        }
        
        if (is_interactive_mode) tcsetpgrp(STDIN_FILENO, getpgrp());
        foreground_pid = -1;
        return pid;
    }

    // Pipeline execution
    int start = 0, num_cmds = pipe_count + 1;
    pid_t pgid = 0, pids[num_cmds];
    int pid_count = 0, fds[2], in_fd = -1;

    for (int i = 0; i < num_cmds; i++) {
        int end = start;
        while(end < token_count && strcmp(tokens[end], "|") != 0) end++;
        
        bool is_last = (i == num_cmds - 1);
        if (!is_last && pipe(fds) == -1) { perror("pipe"); return -1; }

        pid_t pid = fork();
        if (pid == -1) { perror("fork"); return -1; }
        if (pid == 0) { // Child
            if (is_interactive_mode) {
                setpgid(0, pgid == 0 ? 0 : pgid);
                if (!run_in_background) tcsetpgrp(STDIN_FILENO, getpgrp());
            }
            if (in_fd != -1) { dup2(in_fd, STDIN_FILENO); close(in_fd); }
            if (!is_last) { dup2(fds[1], STDOUT_FILENO); close(fds[0]); close(fds[1]); }
            
            char** child_tokens = &tokens[start];
            int child_token_count = end - start;
            run_command_in_child(child_tokens, child_token_count, run_in_background, prev_dir, SHELL_HOME_DIR);
        }

        if (pgid == 0) pgid = pid;
        setpgid(pid, pgid);
        pids[pid_count++] = pid;

        if (in_fd != -1) close(in_fd);
        if (!is_last) { close(fds[1]); in_fd = fds[0]; }
        start = end + 1;
    }

    if (run_in_background) {
        if (pgid != 0) add_background_job(pgid, command_name, RUNNING);
        return pgid;
    }
    
    foreground_pid = pgid;
    if (is_interactive_mode && pgid != 0) tcsetpgrp(STDIN_FILENO, pgid);
    
    int status;
    bool stopped = false;
    int processes_to_wait_for = pid_count;
    while (processes_to_wait_for > 0) {
        pid_t child_pid = waitpid(-pgid, &status, WUNTRACED);
        if (child_pid > 0) {
            if (WIFSTOPPED(status)) {
                stopped = true;
                break;
            } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
                processes_to_wait_for--;
            }
        } else if (errno == ECHILD) {
            break;
        }
    }

    if (stopped) {
        add_background_job(pgid, command_name, STOPPED);
        fprintf(stderr, "\n[%d] Stopped %s\n", background_jobs[background_job_count-1].job_number, command_name);
    }
    
    if (is_interactive_mode) tcsetpgrp(STDIN_FILENO, getpgrp());
    foreground_pid = -1;
    return pgid;
}

bool execute(char** tokens, int token_count, char** prev_dir, char* SHELL_HOME_DIR) {
    if (token_count <= 0) return false;

    check_background_jobs();

    int cmd_start = 0;
    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], ";") == 0 || strcmp(tokens[i], "&") == 0) {
            bool background = (strcmp(tokens[i], "&") == 0);
            tokens[i] = NULL;
            const char* command_name = (cmd_start < i && tokens[cmd_start]) ? tokens[cmd_start] : "";
            execute_pipeline(&tokens[cmd_start], i - cmd_start, background, command_name, prev_dir, SHELL_HOME_DIR);
            cmd_start = i + 1;
        }
    }

    if (cmd_start < token_count) {
        const char* command_name = (tokens[cmd_start]) ? tokens[cmd_start] : "";
        execute_pipeline(&tokens[cmd_start], token_count - cmd_start, false, command_name, prev_dir, SHELL_HOME_DIR);
    }

    return true;
}
