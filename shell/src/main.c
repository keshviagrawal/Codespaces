#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include "../include/parser.h"
#include "../include/hop.h"
#include "../include/reveal.h"
#include "../include/log.h"
#include "../include/executor.h"

bool is_interactive_mode = true;
pid_t foreground_pid = -1;

#define MAX_BUFFER_SIZE 4096

char SHELL_HOME_DIR[MAX_BUFFER_SIZE];

void display_prompt() {
    char hostname[MAX_BUFFER_SIZE];
    char cwd[MAX_BUFFER_SIZE];
    char display_path[MAX_BUFFER_SIZE];

    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw == NULL) {
        perror("getpwuid failed");
        return;
    }
    char *username = pw->pw_name;
    
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("gethostname failed");
        return;
    }
    
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd failed");
        return;
    }

    if (strstr(cwd, SHELL_HOME_DIR) == cwd) {
        strcpy(display_path, "~");
        strcat(display_path, cwd + strlen(SHELL_HOME_DIR));
    } else {
        strncpy(display_path, cwd, sizeof(display_path));
    }
    
    printf("<%s@%s:%s> ", username, hostname, display_path);
    fflush(stdout);
}

void handle_sigint(int signo) {
    (void)signo;
    if (foreground_pid != -1) {
        if (kill(-foreground_pid, SIGINT) == -1) {
            perror("kill failed in SIGINT handler");
        }
    }
}

void handle_sigtstp(int signo) {
    (void)signo;
    if (foreground_pid != -1) {
        if (kill(-foreground_pid, SIGTSTP) == -1) {
            perror("kill failed in SIGTSTP handler");
        }
    }
}

int main() {
    is_interactive_mode = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO) && isatty(STDERR_FILENO);
    
    char *line = NULL;
    size_t len = 0;
    
    char* prev_dir = NULL;

    if (getcwd(SHELL_HOME_DIR, sizeof(SHELL_HOME_DIR)) == NULL) {
        perror("getcwd failed");
        return 1;
    }

    if (is_interactive_mode) {
        signal(SIGINT, handle_sigint);
        signal(SIGTSTP, handle_sigtstp);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
    } else {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
    }
    
    init_log();

    while (1) {
        if (is_interactive_mode) {
            display_prompt();
        }
        
        ssize_t rd = getline(&line, &len, stdin);
        
        if (rd == -1) {
            if (feof(stdin)) {
                check_and_kill_all_jobs();
                printf("\nlogout\n");
                exit(0);
            } else if (errno == EINTR) {
                printf("\n");
                continue;
            } else {
                perror("getline");
                exit(1);
            }
        }

        line[strcspn(line, "\n")] = '\0';
        
        if (is_interactive_mode) {
            check_background_jobs();
        }

        if (strlen(line) == 0 || strspn(line, " \t\n\r") == strlen(line)) {
            continue;
        }

        add_to_log(line);

        if (!parse_input(line)) {
            printf("Invalid Syntax!\n");
            continue;
        }

        char* line_copy = strdup(line);
        if (!line_copy) {
            perror("strdup");
            continue;
        }

        char* tokens[1024];
        int token_count = 0;
        tokenize_input(line_copy, tokens, &token_count);

        if (token_count > 0) {
            char* expanded_args[1024];
            bool needs_free[1024] = {false};

            for (int i = 0; i < token_count; i++) {
                if (tokens[i][0] == '~') {
                    char buffer[4096];
                    snprintf(buffer, sizeof(buffer), "%s%s", SHELL_HOME_DIR, tokens[i] + 1);
                    expanded_args[i] = strdup(buffer);
                    needs_free[i] = true;
                } else {
                    expanded_args[i] = tokens[i];
                }
            }
            expanded_args[token_count] = NULL;

            execute(expanded_args, token_count, &prev_dir, SHELL_HOME_DIR);

            for (int i = 0; i < token_count; i++) {
                if (needs_free[i]) {
                    free(expanded_args[i]);
                }
            }
        }

        free(line_copy);


        
    }
    
    free(line);
    if (prev_dir) {
        free(prev_dir);
    }
    return 0;
}