#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <stdbool.h>
#include <sys/types.h>

// Enum for job states
typedef enum {
    RUNNING,
    STOPPED
} JobState;

// Struct for representing a background job
typedef struct {
    int job_number;
    pid_t pid; // This is the process group ID (pgid)
    char command_name[256];
    JobState state;
} BackgroundJob;

// The main execution function that parses and runs commands.
bool execute(char** tokens, int token_count, char** prev_dir, char* SHELL_HOME_DIR);

// Functions for job control and process management
void check_background_jobs(void);
void list_activities(void);
void ping(pid_t pid, int signal_number);
void check_and_kill_all_jobs(void);
void fg_command(char** tokens, int token_count);
void bg_command(char** tokens, int token_count);
void remove_job_by_pid(pid_t pid);


#endif // EXECUTOR_H
