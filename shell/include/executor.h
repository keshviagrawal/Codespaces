
// #ifndef EXECUTOR_H
// #define EXECUTOR_H

// #include <stdbool.h>
// #include <sys/types.h>

// extern bool is_interactive_mode;

// bool execute_command(char** tokens, int token_count);
// void check_background_jobs(void);
// void list_activities(void);
// void ping(pid_t pid, int signal_number);
// void check_and_kill_all_jobs(void); // NEW: Function to kill all jobs on exit

// #endif // EXECUTOR_H



#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <stdbool.h>
#include <sys/types.h>

// Struct definition is moved here to be shared
typedef struct {
    int job_number;
    pid_t pid;
    char command_name[256];
    char state[20];
} BackgroundJob;

extern bool is_interactive_mode;

bool execute_command(char** tokens, int token_count);
void check_background_jobs(void);
void list_activities(void);
void ping(pid_t pid, int signal_number);
void check_and_kill_all_jobs(void);
void fg_command(char** tokens, int token_count);
void bg_command(char** tokens, int token_count);

#endif // EXECUTOR_H


#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <stdbool.h>
#include <sys/types.h>

// Struct definition is moved here to be shared
typedef struct {
    int job_number;
    pid_t pid;
    char command_name[256];
    char state[20];
} BackgroundJob;

extern bool is_interactive_mode;

bool execute_command(char** tokens, int token_count);
void check_background_jobs(void);
void list_activities(void);
void ping(pid_t pid, int signal_number);
void check_and_kill_all_jobs(void);
void fg_command(char** tokens, int token_count);
void bg_command(char** tokens, int token_count);

#endif // EXECUTOR_H
