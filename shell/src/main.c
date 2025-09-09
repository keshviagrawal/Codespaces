// #define _POSIX_C_SOURCE 200809L
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <string.h>
// #include <sys/types.h>
// #include <signal.h>
// #include <pwd.h>
// #include <fcntl.h>
// #include <signal.h> // NEW: for signal handling and kill()
// #include "../include/parser.h"
// #include "../include/hop.h"
// #include "../include/reveal.h"
// #include "../include/log.h"
// #include "../include/input_redir.h"
// #include "../include/executor.h"

// bool is_interactive_mode = true;

// #define MAX_BUFFER_SIZE 4096

// // Global variable to store the shell's home directory.
// // This is set once at the start of the program.
// char SHELL_HOME_DIR[MAX_BUFFER_SIZE];

// // NEW: Global array to keep track of child processes.
// // This is a simple implementation. A more robust one would be a linked list.
// #define MAX_CHILDREN 100
// pid_t child_pids[MAX_CHILDREN];
// int child_count = 0;

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

// // extern bool is_interactive_mode;

// int main() {

//     //is_interactive_mode = getenv("IS_INTERACTIVE_MODE") == NULL;
//     is_interactive_mode = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO) && isatty(STDERR_FILENO);
    
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
//             // NEW: Ctrl-D detected (EOF)
//             printf("\nlogout\n");
//             // NEW: Send SIGKILL to all tracked child processes
//             for (int i = 0; i < child_count; i++) {
//                 if (child_pids[i] > 0) {
//                     kill(child_pids[i], SIGKILL);
//                 }
//             }
//             exit(0);
//         }

//         line[strcspn(line, "\n")] = '\0';

//         check_background_jobs();
        
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

//                     else if (strcmp(tokens[0], "reveal") == 0) {
//                         int saved_stdin = dup(STDIN_FILENO);
//                         int saved_stdout = dup(STDOUT_FILENO);
//                         int in_fd = -1, out_fd = -1;

//                         // Build filtered args for reveal (exclude redirection operators/filenames)
//                         char* filtered_args[100];
//                         int filtered_count = 0;

//                         for (int i = 1; i < token_count; i++) {
//                             if (tokens[i] == NULL) {
//                                 continue;
//                             }
//                             if (strcmp(tokens[i], "<") == 0) {
//                                 if (i + 1 >= token_count || tokens[i+1] == NULL) {
//                                     fprintf(stderr, "syntax error near unexpected token `<'\n");
//                                     goto restore_fds_reveal;
//                                 }
//                                 in_fd = open(tokens[i+1], O_RDONLY);
//                                 if (in_fd == -1) {
//                                     fprintf(stderr, "No such file or directory!\n");
//                                     goto restore_fds_reveal;
//                                 }
//                                 i++; // skip filename
//                             } else if (strcmp(tokens[i], ">") == 0) {
//                                 if (i + 1 >= token_count || tokens[i+1] == NULL) {
//                                     fprintf(stderr, "syntax error near unexpected token `>'\n");
//                                     goto restore_fds_reveal;
//                                 }
//                                 out_fd = open(tokens[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
//                                 if (out_fd == -1) {
//                                     perror(tokens[i+1]);
//                                     goto restore_fds_reveal;
//                                 }
//                                 i++; // skip filename
//                             } else if (strcmp(tokens[i], ">>") == 0) {
//                                 if (i + 1 >= token_count || tokens[i+1] == NULL) {
//                                     fprintf(stderr, "syntax error near unexpected token `>>'\n");
//                                     goto restore_fds_reveal;
//                                 }
//                                 out_fd = open(tokens[i+1], O_WRONLY | O_CREAT | O_APPEND, 0666);
//                                 if (out_fd == -1) {
//                                     perror(tokens[i+1]);
//                                     goto restore_fds_reveal;
//                                 }
//                                 i++; // skip filename
//                             } else {
//                                 // normal arg (flags, path, or “-”)
//                                 filtered_args[filtered_count++] = tokens[i];
//                             }
//                         }

//                         if (in_fd != -1) {
//                             dup2(in_fd, STDIN_FILENO);
//                             close(in_fd);
//                             in_fd = -1;
//                         }
//                         if (out_fd != -1) {
//                             dup2(out_fd, STDOUT_FILENO);
//                             close(out_fd);
//                             out_fd = -1;
//                         }

//                         reveal(filtered_args, filtered_count, &prev_dir, SHELL_HOME_DIR);

//                     restore_fds_reveal:
//                         if (saved_stdin != -1) {
//                             dup2(saved_stdin, STDIN_FILENO);
//                             close(saved_stdin);
//                         }
//                         if (saved_stdout != -1) {
//                             dup2(saved_stdout, STDOUT_FILENO);
//                             close(saved_stdout);
//                         }
//                     }

//                     // Check for the log command
//                     else if (strcmp(tokens[0], "log") == 0) {
//                         // The log command is not added to the log history itself,
//                         // as per the requirements.
//                         handle_log_command(&tokens[1], token_count - 1, &prev_dir, SHELL_HOME_DIR);
//                     }
                    
//                     // NEW: Check for the activities command
//                     else if (strcmp(tokens[0], "activities") == 0) {
//                         list_activities();
//                     }
                    
//                     else {
//                         // NEW: If it's not a built-in command, execute it using a new function
//                         // Note: This function will be implemented in a future file
//                         // that handles sequential execution.
//                         execute_command(tokens, token_count);
//                     }
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


// #define _POSIX_C_SOURCE 200809L
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <string.h>
// #include <sys/types.h>
// #include <pwd.h>
// #include <fcntl.h>
// #include <signal.h> // NEW: for signal handling and kill()
// #include "../include/parser.h"
// #include "../include/hop.h"
// #include "../include/reveal.h"
// #include "../include/log.h"
// #include "../include/input_redir.h"
// #include "../include/executor.h"

// bool is_interactive_mode = true;

// #define MAX_BUFFER_SIZE 4096

// // Global variable to store the shell's home directory.
// // This is set once at the start of the program.
// char SHELL_HOME_DIR[MAX_BUFFER_SIZE];

// // NEW: Global array to keep track of child processes.
// // This is a simple implementation. A more robust one would be a linked list.
// #define MAX_CHILDREN 100
// pid_t child_pids[MAX_CHILDREN];
// int child_count = 0;

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

// // extern bool is_interactive_mode;

// int main() {

//     //is_interactive_mode = getenv("IS_INTERACTIVE_MODE") == NULL;
//     is_interactive_mode = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO) && isatty(STDERR_FILENO);
    
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
//             // NEW: Ctrl-D detected (EOF)
//             printf("\nlogout\n");
//             // NEW: Send SIGKILL to all tracked child processes
//             for (int i = 0; i < child_count; i++) {
//                 if (child_pids[i] > 0) {
//                     kill(child_pids[i], SIGKILL);
//                 }
//             }
//             exit(0);
//         }

//         line[strcspn(line, "\n")] = '\0';

//         check_background_jobs();
        
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

//                     else if (strcmp(tokens[0], "reveal") == 0) {
//                         int saved_stdin = dup(STDIN_FILENO);
//                         int saved_stdout = dup(STDOUT_FILENO);
//                         int in_fd = -1, out_fd = -1;

//                         // Build filtered args for reveal (exclude redirection operators/filenames)
//                         char* filtered_args[100];
//                         int filtered_count = 0;

//                         for (int i = 1; i < token_count; i++) {
//                             if (tokens[i] == NULL) {
//                                 continue;
//                             }
//                             if (strcmp(tokens[i], "<") == 0) {
//                                 if (i + 1 >= token_count || tokens[i+1] == NULL) {
//                                     fprintf(stderr, "syntax error near unexpected token `<'\n");
//                                     goto restore_fds_reveal;
//                                 }
//                                 in_fd = open(tokens[i+1], O_RDONLY);
//                                 if (in_fd == -1) {
//                                     fprintf(stderr, "No such file or directory!\n");
//                                     goto restore_fds_reveal;
//                                 }
//                                 i++; // skip filename
//                             } else if (strcmp(tokens[i], ">") == 0) {
//                                 if (i + 1 >= token_count || tokens[i+1] == NULL) {
//                                     fprintf(stderr, "syntax error near unexpected token `>'\n");
//                                     goto restore_fds_reveal;
//                                 }
//                                 out_fd = open(tokens[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
//                                 if (out_fd == -1) {
//                                     perror(tokens[i+1]);
//                                     goto restore_fds_reveal;
//                                 }
//                                 i++; // skip filename
//                             } else if (strcmp(tokens[i], ">>") == 0) {
//                                 if (i + 1 >= token_count || tokens[i+1] == NULL) {
//                                     fprintf(stderr, "syntax error near unexpected token `>>'\n");
//                                     goto restore_fds_reveal;
//                                 }
//                                 out_fd = open(tokens[i+1], O_WRONLY | O_CREAT | O_APPEND, 0666);
//                                 if (out_fd == -1) {
//                                     perror(tokens[i+1]);
//                                     goto restore_fds_reveal;
//                                 }
//                                 i++; // skip filename
//                             } else {
//                                 // normal arg (flags, path, or “-”)
//                                 filtered_args[filtered_count++] = tokens[i];
//                             }
//                         }

//                         if (in_fd != -1) {
//                             dup2(in_fd, STDIN_FILENO);
//                             close(in_fd);
//                             in_fd = -1;
//                         }
//                         if (out_fd != -1) {
//                             dup2(out_fd, STDOUT_FILENO);
//                             close(out_fd);
//                             out_fd = -1;
//                         }

//                         reveal(filtered_args, filtered_count, &prev_dir, SHELL_HOME_DIR);

//                     restore_fds_reveal:
//                         if (saved_stdin != -1) {
//                             dup2(saved_stdin, STDIN_FILENO);
//                             close(saved_stdin);
//                         }
//                         if (saved_stdout != -1) {
//                             dup2(saved_stdout, STDOUT_FILENO);
//                             close(saved_stdout);
//                         }
//                     }

//                     // Check for the log command
//                     else if (strcmp(tokens[0], "log") == 0) {
//                         // The log command is not added to the log history itself,
//                         // as per the requirements.
//                         handle_log_command(&tokens[1], token_count - 1, &prev_dir, SHELL_HOME_DIR);
//                     }
                    
//                     // NEW: Check for the activities command
//                     else if (strcmp(tokens[0], "activities") == 0) {
//                         list_activities();
//                     }
                    
//                     // NEW: Check for the ping command
//                     else if (strcmp(tokens[0], "ping") == 0) {
//                         if (token_count != 3) {
//                             fprintf(stderr, "Syntax: ping <pid> <signal_number>\n");
//                         } else {
//                             pid_t pid = (pid_t)strtol(tokens[1], NULL, 10);
//                             int signal_num = (int)strtol(tokens[2], NULL, 10);
//                             ping(pid, signal_num);
//                         }
//                     }

//                     else {
//                         // NEW: If it's not a built-in command, execute it using a new function
//                         // Note: This function will be implemented in a future file
//                         // that handles sequential execution.
//                         execute_command(tokens, token_count);
//                     }
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





// #define _POSIX_C_SOURCE 200809L
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <string.h>
// #include <sys/types.h>
// #include <sys/wait.h>
// #include <pwd.h>
// #include <fcntl.h>
// #include <signal.h>
// #include "../include/parser.h"
// #include "../include/hop.h"
// #include "../include/reveal.h"
// #include "../include/log.h"
// #include "../include/input_redir.h"
// #include "../include/executor.h"

// bool is_interactive_mode = true;
// pid_t foreground_pid = -1; // Global variable to track the foreground process PID

// #define MAX_BUFFER_SIZE 4096

// // Global variable to store the shell's home directory.
// char SHELL_HOME_DIR[MAX_BUFFER_SIZE];

// void display_prompt() {
//     char hostname[MAX_BUFFER_SIZE];
//     char cwd[MAX_BUFFER_SIZE];
//     char display_path[MAX_BUFFER_SIZE];

//     uid_t uid = geteuid();
//     struct passwd *pw = getpwuid(uid);
//     if (pw == NULL) {
//         perror("getpwuid failed");
//         return;
//     }
//     char *username = pw->pw_name;
    
//     if (gethostname(hostname, sizeof(hostname)) != 0) {
//         perror("gethostname failed");
//         return;
//     }
    
//     if (getcwd(cwd, sizeof(cwd)) == NULL) {
//         perror("getcwd failed");
//         return;
//     }

//     if (strstr(cwd, SHELL_HOME_DIR) == cwd) {
//         strcpy(display_path, "~");
//         strcat(display_path, cwd + strlen(SHELL_HOME_DIR));
//     } else {
//         strncpy(display_path, cwd, sizeof(display_path));
//     }
    
//     printf("<%s@%s:%s> ", username, hostname, display_path);
//     fflush(stdout);
// }

// // Signal handler for Ctrl-C (SIGINT)
// void handle_sigint(int signo) {
//     if (foreground_pid != -1) {
//         // Send SIGINT to the foreground process group
//         kill(-foreground_pid, SIGINT);
//     }
// }

// // Signal handler for Ctrl-Z (SIGTSTP)
// void handle_sigtstp(int signo) {
//     if (foreground_pid != -1) {
//         // Send SIGTSTP to the foreground process group
//         kill(-foreground_pid, SIGTSTP);
        
//         // This process will now be handled by waitpid and moved to background jobs
//         // The prompt will be displayed once the waitpid returns
//     }
// }

// int main() {
//     is_interactive_mode = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO) && isatty(STDERR_FILENO);
    
//     char *line = NULL;
//     size_t len = 0;
    
//     char* prev_dir = NULL;

//     if (getcwd(SHELL_HOME_DIR, sizeof(SHELL_HOME_DIR)) == NULL) {
//         perror("getcwd failed");
//         return 1;
//     }

//     // Set up signal handlers for interactive mode
//     if (is_interactive_mode) {
//         signal(SIGINT, handle_sigint);
//         signal(SIGTSTP, handle_sigtstp);
        
//         // Ignore SIGTTOU and SIGTTIN to allow background processes to run
//         signal(SIGTTOU, SIG_IGN);
//         signal(SIGTTIN, SIG_IGN);
//     } else {
//         // In non-interactive mode, set signals to default behavior
//         signal(SIGINT, SIG_DFL);
//         signal(SIGTSTP, SIG_DFL);
//     }

//     init_log();

//     while (1) {
//         display_prompt();
        
//         ssize_t rd = getline(&line, &len, stdin);
        
//         if (rd == -1) {
//             // Ctrl-D detected (EOF)
//             printf("\nlogout\n");
//             check_and_kill_all_jobs();
//             exit(0);
//         }

//         line[strcspn(line, "\n")] = '\0';
        
//         // Check for completed background jobs before parsing new input
//         check_background_jobs();

//         if (strlen(line) == 0 || strspn(line, " \t\n\r") == strlen(line)) {
//             continue;
//         }

//         add_to_log(line);

//         if (!parse_input(line)) {
//             printf("Invalid Syntax!\n");
//         } else {
//             char* line_copy = strdup(line);
//             char* tokens[100];
//             int token_count = 0;

//             if (line_copy) {
//                 tokenize_input(line_copy, tokens, &token_count);

//                 if (token_count > 0) {
//                     if (strcmp(tokens[0], "hop") == 0) {
//                         hop(&tokens[1], token_count - 1, &prev_dir, SHELL_HOME_DIR); 
//                     } 
//                     else if (strcmp(tokens[0], "reveal") == 0) {
//                         // ... (reveal logic remains the same)
//                         int saved_stdin = dup(STDIN_FILENO);
//                         int saved_stdout = dup(STDOUT_FILENO);
//                         int in_fd = -1, out_fd = -1;
//                         char* filtered_args[100];
//                         int filtered_count = 0;
//                         for (int i = 1; i < token_count; i++) {
//                             if (tokens[i] == NULL) {
//                                 continue;
//                             }
//                             if (strcmp(tokens[i], "<") == 0) {
//                                 if (i + 1 >= token_count || tokens[i+1] == NULL) {
//                                     fprintf(stderr, "syntax error near unexpected token `<'\n");
//                                     goto restore_fds_reveal;
//                                 }
//                                 in_fd = open(tokens[i+1], O_RDONLY);
//                                 if (in_fd == -1) {
//                                     fprintf(stderr, "No such file or directory!\n");
//                                     goto restore_fds_reveal;
//                                 }
//                                 i++;
//                             } else if (strcmp(tokens[i], ">") == 0) {
//                                 if (i + 1 >= token_count || tokens[i+1] == NULL) {
//                                     fprintf(stderr, "syntax error near unexpected token `>'\n");
//                                     goto restore_fds_reveal;
//                                 }
//                                 out_fd = open(tokens[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
//                                 if (out_fd == -1) {
//                                     perror(tokens[i+1]);
//                                     goto restore_fds_reveal;
//                                 }
//                                 i++;
//                             } else if (strcmp(tokens[i], ">>") == 0) {
//                                 if (i + 1 >= token_count || tokens[i+1] == NULL) {
//                                     fprintf(stderr, "syntax error near unexpected token `>>'\n");
//                                     goto restore_fds_reveal;
//                                 }
//                                 out_fd = open(tokens[i+1], O_WRONLY | O_CREAT | O_APPEND, 0666);
//                                 if (out_fd == -1) {
//                                     perror(tokens[i+1]);
//                                     goto restore_fds_reveal;
//                                 }
//                                 i++;
//                             } else {
//                                 filtered_args[filtered_count++] = tokens[i];
//                             }
//                         }
//                         if (in_fd != -1) {
//                             dup2(in_fd, STDIN_FILENO);
//                             close(in_fd);
//                             in_fd = -1;
//                         }
//                         if (out_fd != -1) {
//                             dup2(out_fd, STDOUT_FILENO);
//                             close(out_fd);
//                             out_fd = -1;
//                         }
//                         reveal(filtered_args, filtered_count, &prev_dir, SHELL_HOME_DIR);
//                     restore_fds_reveal:
//                         if (saved_stdin != -1) {
//                             dup2(saved_stdin, STDIN_FILENO);
//                             close(saved_stdin);
//                         }
//                         if (saved_stdout != -1) {
//                             dup2(saved_stdout, STDOUT_FILENO);
//                             close(saved_stdout);
//                         }
//                     }
//                     else if (strcmp(tokens[0], "log") == 0) {
//                         handle_log_command(&tokens[1], token_count - 1, &prev_dir, SHELL_HOME_DIR);
//                     }
//                     else if (strcmp(tokens[0], "activities") == 0) {
//                         list_activities();
//                     }
//                     else if (strcmp(tokens[0], "ping") == 0) {
//                         if (token_count != 3) {
//                             fprintf(stderr, "Syntax: ping <pid> <signal_number>\n");
//                         } else {
//                             pid_t pid = (pid_t)strtol(tokens[1], NULL, 10);
//                             int signal_num = (int)strtol(tokens[2], NULL, 10);
//                             ping(pid, signal_num);
//                         }
//                     }
//                     else {
//                         execute_command(tokens, token_count);
//                     }
//                 }

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




// #define _POSIX_C_SOURCE 200809L
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <string.h>
// #include <sys/types.h>
// #include <sys/wait.h>
// #include <pwd.h>
// #include <fcntl.h>
// #include <signal.h>
// #include "../include/parser.h"
// #include "../include/hop.h"
// #include "../include/reveal.h"
// #include "../include/log.h"
// #include "../include/input_redir.h"
// #include "../include/executor.h"

// bool is_interactive_mode = true;
// pid_t foreground_pid = -1; // Global variable to track the foreground process PID

// #define MAX_BUFFER_SIZE 4096

// // Global variable to store the shell's home directory.
// char SHELL_HOME_DIR[MAX_BUFFER_SIZE];

// void display_prompt() {
//     char hostname[MAX_BUFFER_SIZE];
//     char cwd[MAX_BUFFER_SIZE];
//     char display_path[MAX_BUFFER_SIZE];

//     uid_t uid = geteuid();
//     struct passwd *pw = getpwuid(uid);
//     if (pw == NULL) {
//         perror("getpwuid failed");
//         return;
//     }
//     char *username = pw->pw_name;
    
//     if (gethostname(hostname, sizeof(hostname)) != 0) {
//         perror("gethostname failed");
//         return;
//     }
    
//     if (getcwd(cwd, sizeof(cwd)) == NULL) {
//         perror("getcwd failed");
//         return;
//     }

//     if (strstr(cwd, SHELL_HOME_DIR) == cwd) {
//         strcpy(display_path, "~");
//         strcat(display_path, cwd + strlen(SHELL_HOME_DIR));
//     } else {
//         strncpy(display_path, cwd, sizeof(display_path));
//     }
    
//     printf("<%s@%s:%s> ", username, hostname, display_path);
//     fflush(stdout);
// }

// // Signal handler for Ctrl-C (SIGINT)
// void handle_sigint(int signo) {
//     (void)signo; // To suppress the unused parameter warning
//     if (foreground_pid != -1) {
//         // Send SIGINT to the foreground process group
//         kill(-foreground_pid, SIGINT);
//     }
// }

// // Signal handler for Ctrl-Z (SIGTSTP)
// void handle_sigtstp(int signo) {
//     (void)signo; // To suppress the unused parameter warning
//     if (foreground_pid != -1) {
//         // Send SIGTSTP to the foreground process group
//         kill(-foreground_pid, SIGTSTP);
//     }
// }




// int main() {
//     is_interactive_mode = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO) && isatty(STDERR_FILENO);
    
//     char *line = NULL;
//     size_t len = 0;
    
//     char* prev_dir = NULL;

//     if (getcwd(SHELL_HOME_DIR, sizeof(SHELL_HOME_DIR)) == NULL) {
//         perror("getcwd failed");
//         return 1;
//     }

//     // Set up signal handlers for interactive mode
//     if (is_interactive_mode) {
//         signal(SIGINT, handle_sigint);
//         signal(SIGTSTP, handle_sigtstp);
        
//         // Ignore SIGTTOU and SIGTTIN to allow background processes to run
//         signal(SIGTTOU, SIG_IGN);
//         signal(SIGTTIN, SIG_IGN);
//     } else {
//         // In non-interactive mode, set signals to default behavior
//         signal(SIGINT, SIG_DFL);
//         signal(SIGTSTP, SIG_DFL);
//     }

//     init_log();

//     while (1) {
//         display_prompt();
        
//         ssize_t rd = getline(&line, &len, stdin);
        
//         if (rd == -1) {
//             // Ctrl-D detected (EOF)
//             printf("\nlogout\n");
//             check_and_kill_all_jobs();
//             exit(0);
//         }

//         line[strcspn(line, "\n")] = '\0';
        
//         // Check for completed background jobs before parsing new input
//         check_background_jobs();

//         if (strlen(line) == 0 || strspn(line, " \t\n\r") == strlen(line)) {
//             continue;
//         }

//         add_to_log(line);

//         if (!parse_input(line)) {
//             printf("Invalid Syntax!\n");
//         } else {
//             char* line_copy = strdup(line);
//             char* tokens[100];
//             int token_count = 0;

//             if (line_copy) {
//                 tokenize_input(line_copy, tokens, &token_count);

//                 if (token_count > 0) {
//                     if (strcmp(tokens[0], "hop") == 0) {
//                         hop(&tokens[1], token_count - 1, &prev_dir, SHELL_HOME_DIR); 
//                     } 
//                     else if (strcmp(tokens[0], "reveal") == 0) {
//                         int saved_stdin = dup(STDIN_FILENO);
//                         int saved_stdout = dup(STDOUT_FILENO);
//                         int in_fd = -1, out_fd = -1;
//                         char* filtered_args[100];
//                         int filtered_count = 0;
//                         for (int i = 1; i < token_count; i++) {
//                             if (tokens[i] == NULL) {
//                                 continue;
//                             }
//                             if (strcmp(tokens[i], "<") == 0) {
//                                 if (i + 1 >= token_count || tokens[i+1] == NULL) {
//                                     fprintf(stderr, "syntax error near unexpected token `<'\n");
//                                     goto restore_fds_reveal;
//                                 }
//                                 in_fd = open(tokens[i+1], O_RDONLY);
//                                 if (in_fd == -1) {
//                                     fprintf(stderr, "No such file or directory!\n");
//                                     goto restore_fds_reveal;
//                                 }
//                                 i++;
//                             } else if (strcmp(tokens[i], ">") == 0) {
//                                 if (i + 1 >= token_count || tokens[i+1] == NULL) {
//                                     fprintf(stderr, "syntax error near unexpected token `>'\n");
//                                     goto restore_fds_reveal;
//                                 }
//                                 out_fd = open(tokens[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
//                                 if (out_fd == -1) {
//                                     perror(tokens[i+1]);
//                                     goto restore_fds_reveal;
//                                 }
//                                 i++;
//                             } else if (strcmp(tokens[i], ">>") == 0) {
//                                 if (i + 1 >= token_count || tokens[i+1] == NULL) {
//                                     fprintf(stderr, "syntax error near unexpected token `>>'\n");
//                                     goto restore_fds_reveal;
//                                 }
//                                 out_fd = open(tokens[i+1], O_WRONLY | O_CREAT | O_APPEND, 0666);
//                                 if (out_fd == -1) {
//                                     perror(tokens[i+1]);
//                                     goto restore_fds_reveal;
//                                 }
//                                 i++;
//                             } else {
//                                 filtered_args[filtered_count++] = tokens[i];
//                             }
//                         }
//                         if (in_fd != -1) {
//                             dup2(in_fd, STDIN_FILENO);
//                             close(in_fd);
//                             in_fd = -1;
//                         }
//                         if (out_fd != -1) {
//                             dup2(out_fd, STDOUT_FILENO);
//                             close(out_fd);
//                             out_fd = -1;
//                         }
//                         reveal(filtered_args, filtered_count, &prev_dir, SHELL_HOME_DIR);
//                     restore_fds_reveal:
//                         if (saved_stdin != -1) {
//                             dup2(saved_stdin, STDIN_FILENO);
//                             close(saved_stdin);
//                         }
//                         if (saved_stdout != -1) {
//                             dup2(saved_stdout, STDOUT_FILENO);
//                             close(saved_stdout);
//                         }
//                     }
//                     else if (strcmp(tokens[0], "log") == 0) {
//                         handle_log_command(&tokens[1], token_count - 1, &prev_dir, SHELL_HOME_DIR);
//                     }
//                     else if (strcmp(tokens[0], "activities") == 0) {
//                         list_activities();
//                     }
//                     else if (strcmp(tokens[0], "ping") == 0) {
//                         if (token_count != 3) {
//                             fprintf(stderr, "Syntax: ping <pid> <signal_number>\n");
//                         } else {
//                             pid_t pid = (pid_t)strtol(tokens[1], NULL, 10);
//                             int signal_num = (int)strtol(tokens[2], NULL, 10);
//                             ping(pid, signal_num);
//                         }
//                     }
//                     else {
//                         execute_command(tokens, token_count);
//                     }
//                 }

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
#include <sys/wait.h>
#include <pwd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h> // Include errno.h
#include "../include/parser.h"
#include "../include/hop.h"
#include "../include/reveal.h"
#include "../include/log.h"
#include "../include/input_redir.h"
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
        kill(-foreground_pid, SIGINT);
    }
}

void handle_sigtstp(int signo) {
    (void)signo;
    if (foreground_pid != -1) {
        kill(-foreground_pid, SIGTSTP);
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
        display_prompt();
        
        ssize_t rd = getline(&line, &len, stdin);
        
        if (rd == -1) {
            if (errno == EINTR) {
                // Interrupted by a signal, continue the loop
                printf("\n");
                continue;
            } else {
                printf("\nlogout\n");
                check_and_kill_all_jobs();
                exit(0);
            }
        }

        line[strcspn(line, "\n")] = '\0';
        
        check_background_jobs();

        if (strlen(line) == 0 || strspn(line, " \t\n\r") == strlen(line)) {
            continue;
        }

        add_to_log(line);

        if (!parse_input(line)) {
            printf("Invalid Syntax!\n");
        } else {
            char* line_copy = strdup(line);
            char* tokens[100];
            int token_count = 0;

            if (line_copy) {
                tokenize_input(line_copy, tokens, &token_count);

                if (token_count > 0) {
                    if (strcmp(tokens[0], "hop") == 0) {
                        hop(&tokens[1], token_count - 1, &prev_dir, SHELL_HOME_DIR); 
                    } 
                    else if (strcmp(tokens[0], "reveal") == 0) {
                        int saved_stdin = dup(STDIN_FILENO);
                        int saved_stdout = dup(STDOUT_FILENO);
                        int in_fd = -1, out_fd = -1;
                        char* filtered_args[100];
                        int filtered_count = 0;
                        for (int i = 1; i < token_count; i++) {
                            if (tokens[i] == NULL) {
                                continue;
                            }
                            if (strcmp(tokens[i], "<") == 0) {
                                if (i + 1 >= token_count || tokens[i+1] == NULL) {
                                    fprintf(stderr, "syntax error near unexpected token `<'\n");
                                    goto restore_fds_reveal;
                                }
                                in_fd = open(tokens[i+1], O_RDONLY);
                                if (in_fd == -1) {
                                    fprintf(stderr, "No such file or directory!\n");
                                    goto restore_fds_reveal;
                                }
                                i++;
                            } else if (strcmp(tokens[i], ">") == 0) {
                                if (i + 1 >= token_count || tokens[i+1] == NULL) {
                                    fprintf(stderr, "syntax error near unexpected token `>'\n");
                                    goto restore_fds_reveal;
                                }
                                out_fd = open(tokens[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                                if (out_fd == -1) {
                                    perror(tokens[i+1]);
                                    goto restore_fds_reveal;
                                }
                                i++;
                            } else if (strcmp(tokens[i], ">>") == 0) {
                                if (i + 1 >= token_count || tokens[i+1] == NULL) {
                                    fprintf(stderr, "syntax error near unexpected token `>>'\n");
                                    goto restore_fds_reveal;
                                }
                                out_fd = open(tokens[i+1], O_WRONLY | O_CREAT | O_APPEND, 0666);
                                if (out_fd == -1) {
                                    perror(tokens[i+1]);
                                    goto restore_fds_reveal;
                                }
                                i++;
                            } else {
                                filtered_args[filtered_count++] = tokens[i];
                            }
                        }
                        if (in_fd != -1) {
                            dup2(in_fd, STDIN_FILENO);
                            close(in_fd);
                            in_fd = -1;
                        }
                        if (out_fd != -1) {
                            dup2(out_fd, STDOUT_FILENO);
                            close(out_fd);
                            out_fd = -1;
                        }
                        reveal(filtered_args, filtered_count, &prev_dir, SHELL_HOME_DIR);
                    restore_fds_reveal:
                        if (saved_stdin != -1) {
                            dup2(saved_stdin, STDIN_FILENO);
                            close(saved_stdin);
                        }
                        if (saved_stdout != -1) {
                            dup2(saved_stdout, STDOUT_FILENO);
                            close(saved_stdout);
                        }
                    }
                    else if (strcmp(tokens[0], "log") == 0) {
                        handle_log_command(&tokens[1], token_count - 1, &prev_dir, SHELL_HOME_DIR);
                    }
                    else if (strcmp(tokens[0], "activities") == 0) {
                        list_activities();
                    }
                    else if (strcmp(tokens[0], "ping") == 0) {
                        if (token_count != 3) {
                            fprintf(stderr, "Syntax: ping <pid> <signal_number>\n");
                        } else {
                            pid_t pid = (pid_t)strtol(tokens[1], NULL, 10);
                            int signal_num = (int)strtol(tokens[2], NULL, 10);
                            ping(pid, signal_num);
                        }
                    }
                    else {
                        execute_command(tokens, token_count);
                    }
                }

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