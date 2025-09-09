// #ifndef EXECUTOR_H
// #define EXECUTOR_H

// #include <stdbool.h>

// extern bool is_interactive_mode;


// bool execute_command(char** tokens, int token_count);
// void check_background_jobs(void);
// void list_activities(void);
// void ping(pid_t pid, int signal_number); // NEW: function prototype for the 'ping' command

// #endif // EXECUTOR_H


#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <stdbool.h>
#include <sys/types.h>

extern bool is_interactive_mode;

bool execute_command(char** tokens, int token_count);
void check_background_jobs(void);
void list_activities(void);
void ping(pid_t pid, int signal_number);
void check_and_kill_all_jobs(void); // NEW: Function to kill all jobs on exit

#endif // EXECUTOR_H