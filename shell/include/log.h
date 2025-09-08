#ifndef LOG_H
#define LOG_H

#include <stdbool.h>

void init_log();

void add_to_log(const char* command);

// bool handle_log_command(char** args, int num_args);
bool handle_log_command(char** args, int num_args, char** prev_dir, const char* home_dir);

#endif

