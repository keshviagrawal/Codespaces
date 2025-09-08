// #define _POSIX_C_SOURCE 200809L
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <string.h>
// #include <sys/types.h>
// #include <pwd.h>
// #include "../include/parser.h"
// #include "../include/hop.h"
// #include "../include/reveal.h" 
// #include "../include/log.h"
// #include "../include/input_redir.h"

// #define MAX_BUFFER_SIZE 4096

// // Global variable to store the shell's home directory.
// // This is set once at the start of the program.
// char SHELL_HOME_DIR[MAX_BUFFER_SIZE];

// void display_prompt() {
//     char hostname[MAX_BUFFER_SIZE];
//     char cwd[MAX_BUFFER_SIZE];
//     char display_path[MAX_BUFFER_SIZE];

//     // Get the user's username using geteuid() and getpwuid()
//     uid_t uid = geteuid();
//     struct passwd *pw = getpwuid(uid);
//     if (pw == NULL) {
//         perror("getpwuid failed");
//         return;
//     }
//     char *username = pw->pw_name;
    
//     // Get the system hostname using gethostname()
//     if (gethostname(hostname, sizeof(hostname)) != 0) {
//         perror("gethostname failed");
//         return;
//     }
    
//     // Get the current working directory (CWD)
//     if (getcwd(cwd, sizeof(cwd)) == NULL) {
//         perror("getcwd failed");
//         return;
//     }

//     if (strstr(cwd, SHELL_HOME_DIR) == cwd) {
//         strcpy(display_path, "~");
//         strcat(display_path, cwd + strlen(SHELL_HOME_DIR));
//     } else {
//         // Otherwise, display the full absolute path as is.
//         strncpy(display_path, cwd, sizeof(display_path));
//     }
    
//     // Print the final prompt to the console in the required format
//     printf("<%s@%s:%s> ", username, hostname, display_path);
//     fflush(stdout); // Ensure the prompt is displayed immediately
// }


// int main() {
//     char *line = NULL;
//     size_t len = 0;
    
//     // Variable to store the previous directory, required by the hop function.
//     char* prev_dir = NULL;

//     // IMPORTANT: Capture the current directory when the shell starts
//     if (getcwd(SHELL_HOME_DIR, sizeof(SHELL_HOME_DIR)) == NULL) {
//         perror("getcwd failed");
//         return 1;
//     }

//     // NEW: Initialize the log module by loading history from the file
//     init_log();

//     // The main loop that keeps the shell running indefinitely
//     while (1) {
//         display_prompt();
        
//         ssize_t rd = getline(&line, &len, stdin);
        
//         if (rd == -1) {
//             printf("\n");
//             break;
//         }

//         line[strcspn(line, "\n")] = '\0';
        
//         if (strlen(line) == 0 || strspn(line, " \t\n\r") == strlen(line)) {
//             continue;
//         }

//         add_to_log(line);

//         // Call the parser to validate the syntax
//         if (!parse_input(line)) {
//             printf("Invalid Syntax!\n");
//         } else {
//             // If the syntax is valid, tokenize the input and execute the command.
//             char* line_copy = strdup(line);
//             char* tokens[100];
//             int token_count = 0;

//             if (line_copy) {
//                 tokenize_input(line_copy, tokens, &token_count);

//                 if (token_count > 0) {
//                     // Check for the hop command
//                     if (strcmp(tokens[0], "hop") == 0) {
//                         hop(&tokens[1], token_count - 1, &prev_dir, SHELL_HOME_DIR); 
//                     } 
//                     // Check for the reveal command
//                     else if (strcmp(tokens[0], "reveal") == 0) {
//                         reveal(&tokens[1], token_count - 1, &prev_dir, SHELL_HOME_DIR);
                        
//                     }
//                     // Check for the log command
//                     else if (strcmp(tokens[0], "log") == 0) {
//                         // The log command is not added to the log history itself,
//                         // as per the requirements.
//                         handle_log_command(&tokens[1], token_count - 1);
//                     }

//                     else {
//                         // NEW: If it's not a built-in command, execute it using our new function
//                         execute_command_group(tokens, token_count);
                        
//                     }

//                     // TODO: Add logic for other commands here
//                 }

//                 // Free the memory allocated by the tokenizer
//                 for (int i = 0; i < token_count; i++) {
//                     free(tokens[i]);
//                 }
//                 free(line_copy);
//             }
//         }
//     }
    
//     free(line);
//     if (prev_dir) {
//         free(prev_dir);
//     }
//     return 0;
// }




#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <signal.h> // NEW: for signal handling and kill()
#include "../include/parser.h"
#include "../include/hop.h"
#include "../include/reveal.h"
#include "../include/log.h"
#include "../include/input_redir.h"

#define MAX_BUFFER_SIZE 4096

// Global variable to store the shell's home directory.
// This is set once at the start of the program.
char SHELL_HOME_DIR[MAX_BUFFER_SIZE];

// NEW: Global array to keep track of child processes.
// This is a simple implementation. A more robust one would be a linked list.
#define MAX_CHILDREN 100
pid_t child_pids[MAX_CHILDREN];
int child_count = 0;

void display_prompt() {
    char hostname[MAX_BUFFER_SIZE];
    char cwd[MAX_BUFFER_SIZE];
    char display_path[MAX_BUFFER_SIZE];

    // Get the user's username using geteuid() and getpwuid()
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw == NULL) {
        perror("getpwuid failed");
        return;
    }
    char *username = pw->pw_name;
    
    // Get the system hostname using gethostname()
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("gethostname failed");
        return;
    }
    
    // Get the current working directory (CWD)
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd failed");
        return;
    }

    if (strstr(cwd, SHELL_HOME_DIR) == cwd) {
        strcpy(display_path, "~");
        strcat(display_path, cwd + strlen(SHELL_HOME_DIR));
    } else {
        // Otherwise, display the full absolute path as is.
        strncpy(display_path, cwd, sizeof(display_path));
    }
    
    // Print the final prompt to the console in the required format
    printf("<%s@%s:%s> ", username, hostname, display_path);
    fflush(stdout); // Ensure the prompt is displayed immediately
}


int main() {
    char *line = NULL;
    size_t len = 0;
    
    // Variable to store the previous directory, required by the hop function.
    char* prev_dir = NULL;

    // IMPORTANT: Capture the current directory when the shell starts
    if (getcwd(SHELL_HOME_DIR, sizeof(SHELL_HOME_DIR)) == NULL) {
        perror("getcwd failed");
        return 1;
    }

    // NEW: Initialize the log module by loading history from the file
    init_log();

    // The main loop that keeps the shell running indefinitely
    while (1) {
        display_prompt();
        
        ssize_t rd = getline(&line, &len, stdin);
        
        if (rd == -1) {
            // NEW: Ctrl-D detected (EOF)
            printf("\nlogout\n");
            // NEW: Send SIGKILL to all tracked child processes
            for (int i = 0; i < child_count; i++) {
                if (child_pids[i] > 0) {
                    kill(child_pids[i], SIGKILL);
                }
            }
            exit(0);
        }

        line[strcspn(line, "\n")] = '\0';
        
        if (strlen(line) == 0 || strspn(line, " \t\n\r") == strlen(line)) {
            continue;
        }

        add_to_log(line);

        // Call the parser to validate the syntax
        if (!parse_input(line)) {
            printf("Invalid Syntax!\n");
        } else {
            // If the syntax is valid, tokenize the input and execute the command.
            char* line_copy = strdup(line);
            char* tokens[100];
            int token_count = 0;

            if (line_copy) {
                tokenize_input(line_copy, tokens, &token_count);

                if (token_count > 0) {
                    // Check for the hop command
                    if (strcmp(tokens[0], "hop") == 0) {
                        hop(&tokens[1], token_count - 1, &prev_dir, SHELL_HOME_DIR); 
                    } 
                    // Check for the reveal command
                    else if (strcmp(tokens[0], "reveal") == 0) {
                        reveal(&tokens[1], token_count - 1, &prev_dir, SHELL_HOME_DIR);
                        
                    }
                    // Check for the log command
                    else if (strcmp(tokens[0], "log") == 0) {
                        // The log command is not added to the log history itself,
                        // as per the requirements.
                        handle_log_command(&tokens[1], token_count - 1);
                    }

                    else {
                        // NEW: If it's not a built-in command, execute it using a new function
                        // Note: This function will be implemented in a future file
                        // that handles sequential execution.
                        execute_command_group(tokens, token_count);
                    }

                    // TODO: Add logic for other commands here
                }

                // Free the memory allocated by the tokenizer
                for (int i = 0; i < token_count; i++) {
                    free(tokens[i]);
                }
                free(line_copy);
            }
        }
    }
    
    free(line);
    if (prev_dir) {
        free(prev_dir);
    }
    return 0;
}