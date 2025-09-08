#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <stdbool.h>

extern bool is_interactive_mode;


bool execute_command(char** tokens, int token_count);
void check_background_jobs(void);

#endif // EXECUTOR_H



// #ifndef EXECUTOR_H
// #define EXECUTOR_H

// #include <stdbool.h>

// // Global interactive mode flag
// extern bool is_interactive_mode;

// bool execute_command(char** tokens, int token_count);

// #endif
