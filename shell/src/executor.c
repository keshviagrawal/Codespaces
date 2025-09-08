// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <stdbool.h>
// #include <sys/wait.h>
// #include <sys/types.h>
// #include <fcntl.h>
// #include <errno.h>
// #include "../include/executor.h"

// // Forward declarations for helper functions
// static void execute_single_command(char** tokens);
// static bool execute_pipe_group(char** tokens, int token_count);


// static void execute_single_command(char** tokens) {
//     int saved_stdin = dup(STDIN_FILENO);
//     int saved_stdout = dup(STDOUT_FILENO);

//     int in_fd = -1;
//     int out_fd = -1;
//     char** cmd_args = tokens;
    
//     // Process redirections first
//     for (int i = 0; tokens[i] != NULL; i++) {
//         if (strcmp(tokens[i], "<") == 0) {
//             if (tokens[i+1] == NULL) {
//                 fprintf(stderr, "syntax error near unexpected token `<'\n");
//                 exit(1);
//             }
//             in_fd = open(tokens[i+1], O_RDONLY);
//             if (in_fd == -1) {
//                 perror(tokens[i+1]);
//                 exit(1);
//             }
//             tokens[i] = NULL;
//         } else if (strcmp(tokens[i], ">") == 0) {
//             if (tokens[i+1] == NULL) {
//                 fprintf(stderr, "syntax error near unexpected token `>'\n");
//                 exit(1);
//             }
//             out_fd = open(tokens[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
//             if (out_fd == -1) {
//                 perror(tokens[i+1]);
//                 exit(1);
//             }
//             tokens[i] = NULL;
//         } else if (strcmp(tokens[i], ">>") == 0) {
//             if (tokens[i+1] == NULL) {
//                 fprintf(stderr, "syntax error near unexpected token `>>'\n");
//                 exit(1);
//             }
//             out_fd = open(tokens[i+1], O_WRONLY | O_CREAT | O_APPEND, 0666);
//             if (out_fd == -1) {
//                 perror(tokens[i+1]);
//                 exit(1);
//             }
//             tokens[i] = NULL;
//         }
//     }

//     if (in_fd != -1) {
//         dup2(in_fd, STDIN_FILENO);
//         close(in_fd);
//     }
//     if (out_fd != -1) {
//         dup2(out_fd, STDOUT_FILENO);
//         close(out_fd);
//     }

//     // Execute the command
//     execvp(cmd_args[0], cmd_args);
    
//     // If execvp returns, it must have failed.
//     fprintf(stderr, "%s: Command not found\n", cmd_args[0]);

//     // Restore original file descriptors before exiting.
//     if (in_fd != -1) {
//         dup2(saved_stdin, STDIN_FILENO);
//     }
//     if (out_fd != -1) {
//         dup2(saved_stdout, STDOUT_FILENO);
//     }
//     close(saved_stdin);
//     close(saved_stdout);

//     exit(1);
// }

// /**
//  * @brief Executes a chain of piped commands.
//  * @param tokens The array of strings for the commands.
//  * @param token_count The number of tokens.
//  * @return true on success, false on failure.
//  */
// static bool execute_pipe_group(char** tokens, int token_count) {
//     if (token_count <= 0) {
//         return false;
//     }
    
//     int start = 0;
//     int prev_pipe[2] = {-1, -1};
//     pid_t pids[token_count];
//     int pid_count = 0;

//     for (int i = 0; i <= token_count; i++) {
//         if (i == token_count || strcmp(tokens[i], "|") == 0) {
//             int cur_pipe[2] = {-1, -1};
//             bool is_last = (i == token_count);
//             if (!is_last) {
//                 if (pipe(cur_pipe) == -1) {
//                     perror("pipe failed");
//                     return false;
//                 }
//             }

//             pid_t pid = fork();
//             if (pid == -1) {
//                 perror("fork failed");
//                 return false;
//             } else if (pid == 0) { // Child process
//                 if (prev_pipe[0] != -1) {
//                     dup2(prev_pipe[0], STDIN_FILENO);
//                     close(prev_pipe[0]);
//                     close(prev_pipe[1]);
//                 }
//                 if (!is_last) {
//                     dup2(cur_pipe[1], STDOUT_FILENO);
//                     close(cur_pipe[0]);
//                     close(cur_pipe[1]);
//                 }
                
//                 // Null-terminate the current command for execvp.
//                 tokens[i] = NULL;
//                 execute_single_command(&tokens[start]);
//             } else { // Parent process
//                 pids[pid_count++] = pid;
//                 if (prev_pipe[0] != -1) {
//                     close(prev_pipe[0]);
//                     close(prev_pipe[1]);
//                 }
//                 if (!is_last) {
//                     prev_pipe[0] = cur_pipe[0];
//                     prev_pipe[1] = cur_pipe[1];
//                 }
//             }
//             start = i + 1;
//         }
//     }

//     // Parent waits for all children in the pipe group to finish
//     for (int i = 0; i < pid_count; i++) {
//         waitpid(pids[i], NULL, 0);
//     }
//     return true;
// }

// /**
//  * @brief Executes a sequence of commands separated by semicolons.
//  * @param tokens The array of strings representing the tokens of the command line.
//  * @param token_count The number of tokens.
//  * @return Returns true on success, false on failure.
//  */
// bool execute_command(char** tokens, int token_count) {
//     if (token_count <= 0) {
//         return false;
//     }

//     int cmd_start = 0;
//     for (int i = 0; i < token_count; i++) {
//         if (strcmp(tokens[i], ";") == 0) {
//             tokens[i] = NULL;
//             if (!execute_pipe_group(&tokens[cmd_start], i - cmd_start)) {
//                 return false;
//             }
//             cmd_start = i + 1;
//         }
//     }
    
//     // Execute the final command group after the last semicolon (or the only group).
//     if (cmd_start < token_count) {
//         if (!execute_pipe_group(&tokens[cmd_start], token_count - cmd_start)) {
//             return false;
//         }
//     }

//     return true;
// }


// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <stdbool.h>
// #include <sys/wait.h>
// #include <sys/types.h>
// #include <fcntl.h>
// #include <errno.h>
// #include "../include/executor.h"

// static void execute_single_command(char** tokens, bool run_in_background);
// static pid_t execute_pipe_group(char** tokens, int token_count, bool run_in_background, const char* command_name);

// /* -----------------------
//    Background job tracking
//    ----------------------- */
// typedef struct BackgroundJob {
//     int job_number;
//     pid_t pid;
//     char command_name[256];
// } BackgroundJob;

// static BackgroundJob background_jobs[256];
// static int background_job_count = 0;
// static int next_job_number = 1;

// static void add_background_job(pid_t pid, const char* command_name) {
//     if (background_job_count >= (int)(sizeof(background_jobs) / sizeof(background_jobs[0]))) {
//         return;
//     }
//     BackgroundJob* job = &background_jobs[background_job_count++];
//     job->job_number = next_job_number++;
//     job->pid = pid;
//     if (command_name != NULL) {
//         strncpy(job->command_name, command_name, sizeof(job->command_name) - 1);
//         job->command_name[sizeof(job->command_name) - 1] = '\0';
//     } else {
//         job->command_name[0] = '\0';
//     }
//     fprintf(stderr, "[%d] %d\n", job->job_number, (int)job->pid);
//     fflush(stderr);
// }

// static void remove_background_job_index(int index) {
//     if (index < 0 || index >= background_job_count) {
//         return;
//     }
//     for (int i = index; i < background_job_count - 1; i++) {
//         background_jobs[i] = background_jobs[i + 1];
//     }
//     background_job_count--;
// }

// /* Call this before parsing each new input line */
// void check_background_jobs(void) {
//     int status;
//     for (int i = 0; i < background_job_count; ) {
//         pid_t pid = background_jobs[i].pid;
//         pid_t result = waitpid(pid, &status, WNOHANG);
//         if (result == 0) {
//             i++;
//             continue;
//         } else if (result == -1) {
//             remove_background_job_index(i);
//             continue;
//         } else {
//             const char* name = background_jobs[i].command_name[0] != '\0' ? background_jobs[i].command_name : "";
//             if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
//                 //printf("%s with pid %d exited normally\n", name, (int)pid);
//                 fprintf(stderr, "%s with pid %d exited normally\n", name, (int)pid);
//             } else {
//                 // printf("%s with pid %d exited abnormally\n", name, (int)pid);
//                 fprintf(stderr, "%s with pid %d exited abnormally\n", name, (int)pid);
//             }
//             //fflush(stdout);
//             fflush(stderr);
//             remove_background_job_index(i);
//         }
//     }
// }

// static void execute_single_command(char** tokens, bool run_in_background) {
//     int saved_stdin = dup(STDIN_FILENO);
//     int saved_stdout = dup(STDOUT_FILENO);

//     int in_fd = -1;
//     int out_fd = -1;
//     char** cmd_args = tokens;

//     for (int i = 0; tokens[i] != NULL; i++) {
//         if (strcmp(tokens[i], "<") == 0) {
//             if (tokens[i+1] == NULL) {
//                 fprintf(stderr, "syntax error near unexpected token `<'\n");
//                 exit(1);
//             }
//             in_fd = open(tokens[i+1], O_RDONLY);
//             if (in_fd == -1) {
//                 perror(tokens[i+1]);
//                 exit(1);
//             }
//             tokens[i] = NULL;
//         } else if (strcmp(tokens[i], ">") == 0) {
//             if (tokens[i+1] == NULL) {
//                 fprintf(stderr, "syntax error near unexpected token `>'\n");
//                 exit(1);
//             }
//             out_fd = open(tokens[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
//             if (out_fd == -1) {
//                 perror(tokens[i+1]);
//                 exit(1);
//             }
//             tokens[i] = NULL;
//         } else if (strcmp(tokens[i], ">>") == 0) {
//             if (tokens[i+1] == NULL) {
//                 fprintf(stderr, "syntax error near unexpected token `>>'\n");
//                 exit(1);
//             }
//             out_fd = open(tokens[i+1], O_WRONLY | O_CREAT | O_APPEND, 0666);
//             if (out_fd == -1) {
//                 perror(tokens[i+1]);
//                 exit(1);
//             }
//             tokens[i] = NULL;
//         }
//     }

//     if (in_fd != -1) {
//         dup2(in_fd, STDIN_FILENO);
//         close(in_fd);
//     } else if (run_in_background) {
//         int devnull = open("/dev/null", O_RDONLY);
//         if (devnull != -1) {
//             dup2(devnull, STDIN_FILENO);
//             close(devnull);
//         }
//     }
//     if (out_fd != -1) {
//         dup2(out_fd, STDOUT_FILENO);
//         close(out_fd);
//     }

//     execvp(cmd_args[0], cmd_args);

//     fprintf(stderr, "%s: Command not found\n", cmd_args[0]);

//     if (in_fd != -1) {
//         dup2(saved_stdin, STDIN_FILENO);
//     }
//     if (out_fd != -1) {
//         dup2(saved_stdout, STDOUT_FILENO);
//     }
//     close(saved_stdin);
//     close(saved_stdout);

//     exit(1);
// }

// static pid_t execute_pipe_group(char** tokens, int token_count, bool run_in_background, const char* command_name) {
//     if (token_count <= 0) {
//         return -1;
//     }

//     int start = 0;
//     int prev_pipe[2] = {-1, -1};
//     pid_t pids[token_count];
//     int pid_count = 0;

//     for (int i = 0; i <= token_count; i++) {
//         if (i == token_count || strcmp(tokens[i], "|") == 0) {
//             int cur_pipe[2] = {-1, -1};
//             bool is_last = (i == token_count);
//             if (!is_last) {
//                 if (pipe(cur_pipe) == -1) {
//                     perror("pipe failed");
//                     return -1;
//                 }
//             }

//             pid_t pid = fork();
//             if (pid == -1) {
//                 perror("fork failed");
//                 return -1;
//             } else if (pid == 0) {
//                 if (prev_pipe[0] != -1) {
//                     dup2(prev_pipe[0], STDIN_FILENO);
//                     close(prev_pipe[0]);
//                     close(prev_pipe[1]);
//                 }
//                 if (!is_last) {
//                     dup2(cur_pipe[1], STDOUT_FILENO);
//                     close(cur_pipe[0]);
//                     close(cur_pipe[1]);
//                 }

//                 tokens[i] = NULL;
//                 execute_single_command(&tokens[start], run_in_background);
//             } else {
//                 pids[pid_count++] = pid;
//                 if (prev_pipe[0] != -1) {
//                     close(prev_pipe[0]);
//                     close(prev_pipe[1]);
//                 }
//                 if (!is_last) {
//                     prev_pipe[0] = cur_pipe[0];
//                     prev_pipe[1] = cur_pipe[1];
//                 }
//             }
//             start = i + 1;
//         }
//     }

//     if (run_in_background) {
//         if (pid_count > 0) {
//             add_background_job(pids[pid_count - 1], command_name);
//         }
//         return pid_count > 0 ? pids[pid_count - 1] : -1;
//     }

//     for (int i = 0; i < pid_count; i++) {
//         waitpid(pids[i], NULL, 0);
//     }
//     return pid_count > 0 ? pids[pid_count - 1] : -1;
// }

// bool execute_command(char** tokens, int token_count) {
//     if (token_count <= 0) {
//         return false;
//     }

//     int cmd_start = 0;
//     for (int i = 0; i < token_count; i++) {
//         if (strcmp(tokens[i], ";") == 0 || strcmp(tokens[i], "&") == 0) {
//             bool background = (strcmp(tokens[i], "&") == 0);
//             tokens[i] = NULL;
//             const char* command_name = (cmd_start < token_count && tokens[cmd_start] != NULL) ? tokens[cmd_start] : "";
//             if (execute_pipe_group(&tokens[cmd_start], i - cmd_start, background, command_name) == -1) {
//                 return false;
//             }
//             cmd_start = i + 1;
//         }
//     }

//     if (cmd_start < token_count) {
//         const char* command_name = (tokens[cmd_start] != NULL) ? tokens[cmd_start] : "";
//         if (execute_pipe_group(&tokens[cmd_start], token_count - cmd_start, false, command_name) == -1) {
//             return false;
//         }
//     }

//     return true;
// }


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include "../include/executor.h"
// #include "../include/builtin.h"

static void execute_single_command(char** tokens, bool run_in_background);
static pid_t execute_pipe_group(char** tokens, int token_count, bool run_in_background, const char* command_name);

/* -----------------------
   Background job tracking
   ----------------------- */
typedef struct BackgroundJob {
    int job_number;
    pid_t pid;
    char command_name[256];
} BackgroundJob;

static BackgroundJob background_jobs[256];
static int background_job_count = 0;
static int next_job_number = 1;

static void add_background_job(pid_t pid, const char* command_name) {
    if (background_job_count >= (int)(sizeof(background_jobs) / sizeof(background_jobs[0]))) {
        return;
    }
    BackgroundJob* job = &background_jobs[background_job_count++];
    job->job_number = next_job_number++;
    job->pid = pid;
    if (command_name != NULL) {
        strncpy(job->command_name, command_name, sizeof(job->command_name) - 1);
        job->command_name[sizeof(job->command_name) - 1] = '\0';
    } else {
        job->command_name[0] = '\0';
    }
    /* Print job notice only in interactive terminals */
    // if (isatty(STDERR_FILENO)) {
    //     fprintf(stderr, "[%d] %d\n", job->job_number, (int)job->pid);
    //     fflush(stderr);
    // }

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

/* Call this before parsing each new input line */
void check_background_jobs(void) {
    int status;
    for (int i = 0; i < background_job_count; ) {
        pid_t pid = background_jobs[i].pid;
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == 0) {
            i++;
            continue;
        } else if (result == -1) {
            remove_background_job_index(i);
            continue;
        } else {
            const char* name = background_jobs[i].command_name[0] != '\0' ? background_jobs[i].command_name : "";
            /* Print completion notices only in interactive terminals */
            
            // if (isatty(STDERR_FILENO)) {
            //     if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            //         fprintf(stderr, "%s with pid %d exited normally\n", name, (int)pid);
            //     } else {
            //         fprintf(stderr, "%s with pid %d exited abnormally\n", name, (int)pid);
            //     }
            //     fflush(stderr);
            // }

            if (is_interactive_mode) {
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    fprintf(stderr, "%s with pid %d exited normally\n", name, (int)pid);
                } else {
                    fprintf(stderr, "%s with pid %d exited abnormally\n", name, (int)pid);
                }
                fflush(stderr);
            }
            remove_background_job_index(i);
        }
    }
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
                // perror(tokens[i+1]);
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

    fprintf(stderr, "%s: Command not found\n", cmd_args[0]);





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

    int start = 0;
    int prev_pipe[2] = {-1, -1};
    pid_t pids[token_count];
    int pid_count = 0;

    for (int i = 0; i <= token_count; i++) {
        if (i == token_count || strcmp(tokens[i], "|") == 0) {
            int cur_pipe[2] = {-1, -1};
            bool is_last = (i == token_count);
            if (!is_last) {
                if (pipe(cur_pipe) == -1) {
                    perror("pipe failed");
                    return -1;
                }
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("fork failed");
                return -1;
            } else if (pid == 0) {
                if (prev_pipe[0] != -1) {
                    dup2(prev_pipe[0], STDIN_FILENO);
                    close(prev_pipe[0]);
                    close(prev_pipe[1]);
                }
                if (!is_last) {
                    dup2(cur_pipe[1], STDOUT_FILENO);
                    close(cur_pipe[0]);
                    close(cur_pipe[1]);
                }

                tokens[i] = NULL;
                execute_single_command(&tokens[start], run_in_background);
            } else {
                pids[pid_count++] = pid;
                if (prev_pipe[0] != -1) {
                    close(prev_pipe[0]);
                    close(prev_pipe[1]);
                }
                if (!is_last) {
                    prev_pipe[0] = cur_pipe[0];
                    prev_pipe[1] = cur_pipe[1];
                }
            }
            start = i + 1;
        }
    }

    if (run_in_background) {
        if (pid_count > 0) {
            add_background_job(pids[pid_count - 1], command_name);
        }
        return pid_count > 0 ? pids[pid_count - 1] : -1;
    }

    for (int i = 0; i < pid_count; i++) {
        waitpid(pids[i], NULL, 0);
    }
    return pid_count > 0 ? pids[pid_count - 1] : -1;
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