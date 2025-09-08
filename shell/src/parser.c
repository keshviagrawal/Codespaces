// #define _POSIX_C_SOURCE 200809L
// #include <stdbool.h>
// #include <string.h>
// #include <stdlib.h>
// #include <stdio.h>
// #include "../include/parser.h"

// #define MAX_TOKENS 100
// #define MAX_TOKEN_LENGTH 256

// // Helper function to remove leading and trailing whitespace.
// void trim_whitespace(char* str) {
//     if (str == NULL || *str == '\0') {
//         return;
//     }

//     char* start = str;
//     while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') {
//         start++;
//     }

//     char* end = str + strlen(start) - 1;
//     while (end > start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
//         end--;
//     }

//     *(end + 1) = '\0';
//     memmove(str, start, strlen(start) + 1);
// }

// // Check if a token is a valid command name (not an operator).
// bool is_command_or_arg(const char* token) {
//     if (token == NULL || strlen(token) == 0) return false;
//     return (strpbrk(token, "|&><;") == NULL);
// }

// // Check if a token is one of the valid operators.
// bool is_valid_operator(const char* token) {
//     return (strcmp(token, "|") == 0 || strcmp(token, ";") == 0 ||
//             strcmp(token, "<") == 0 || strcmp(token, ">") == 0 ||
//             strcmp(token, ">>") == 0 || strcmp(token, "&") == 0);
// }

// // A robust tokenizer that correctly separates operators and arguments by copying them.
// void tokenize_input(char* line, char* tokens[], int* token_count) {
//     *token_count = 0;
//     char* current = line;

//     while (*current != '\0' && *token_count < MAX_TOKENS) {
//         // Skip leading whitespace.
//         while (*current == ' ' || *current == '\t' || *current == '\n' || *current == '\r') {
//             current++;
//         }
//         if (*current == '\0') break;

//         char token_buffer[MAX_TOKEN_LENGTH];
//         int buffer_index = 0;
        
//         // Handle two-character operators first to prevent issues with single-character operators.
//         if (*current == '>' && current[1] == '>') {
//             tokens[(*token_count)++] = strdup(">>");
//             current += 2;
//             continue;
//         }

//         // Handle one-character operators.
//         if (strchr("|&><;", *current) != NULL) {
//             token_buffer[buffer_index++] = *current;
//             token_buffer[buffer_index] = '\0';
//             tokens[(*token_count)++] = strdup(token_buffer);
//             current++;
//             continue;
//         }

//         // Otherwise, it's a word (command or arg).
//         char* token_start = current;
//         while (*current != '\0' && !strchr(" |&><;", *current) &&
//                *current != '\t' && *current != '\n' && *current != '\r') {
//             current++;
//         }
//         int len = current - token_start;
//         strncpy(token_buffer, token_start, len);
//         token_buffer[len] = '\0';
//         tokens[(*token_count)++] = strdup(token_buffer);
//     }
// }

// // This function validates a single atomic command group (between | or ;).
// bool validate_atomic_command(char* tokens[], int start_idx, int end_idx) {
//     if (start_idx > end_idx) {
//         return false;
//     }
    
//     // An atomic command must start with a valid command name.
//     if (!is_command_or_arg(tokens[start_idx])) {
//         return false;
//     }

//     // Iterate through the tokens in the atomic command to check for invalid sequences.
//     for (int i = start_idx; i <= end_idx; i++) {
//         char* current_token = tokens[i];
        
//         // Redirection operators must be followed by a command/arg (a name).
//         if (strcmp(current_token, "<") == 0 || strcmp(current_token, ">") == 0 || strcmp(current_token, ">>") == 0) {
//             // Check if the next token is a valid name.
//             if (i + 1 > end_idx || !is_command_or_arg(tokens[i+1])) {
//                 return false;
//             }
//         }
//     }

//     return true;
// }

// // The main parsing function.
// bool parse_input(char* line) {
//     if (line == NULL) return false;

//     char* line_copy = strdup(line);
//     if (!line_copy) {
//         perror("strdup");
//         return false;
//     }

//     trim_whitespace(line_copy);
//     if (strlen(line_copy) == 0) {
//         free(line_copy);
//         return true;
//     }

//     char* tokens[MAX_TOKENS];
//     int token_count = 0;
//     tokenize_input(line_copy, tokens, &token_count);

//     if (token_count == 0) {
//         free(line_copy);
//         return true;
//     }

//     int current_cmd_start = 0;
    
//     // The main loop that iterates over command groups.
//     for (int i = 0; i < token_count; i++) {
//         char* current_token = tokens[i];

//         if (strcmp(current_token, ";") == 0 || strcmp(current_token, "|") == 0) {
//             // Validate the command group before the operator.
//             if (!validate_atomic_command(tokens, current_cmd_start, i - 1)) {
//                 free(line_copy);
//                 return false;
//             }
//             // An operator cannot be at the end of the input (checked later)
//             // or followed by another operator.
//             if (i + 1 >= token_count || is_valid_operator(tokens[i+1])) {
//                 free(line_copy);
//                 return false;
//             }
//             current_cmd_start = i + 1; // Start of the next command group.
//         } else if (strcmp(current_token, "&") == 0) {
//             if (i != token_count - 1) { // & must be the very last token
//                 free(line_copy);
//                 return false;
//             }
//         } else {
//             // Every token must be valid.
//             if (!is_command_or_arg(current_token) && !is_valid_operator(current_token)) {
//                  free(line_copy);
//                  return false;
//             }
//         }
//     }
    
//     // Validate the final command group.
//     if (!validate_atomic_command(tokens, current_cmd_start, token_count - 1)) {
//         free(line_copy);
//         return false;
//     }

//     free(line_copy);
//     return true;
// }


#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../include/parser.h"

#define MAX_TOKENS 100
#define MAX_TOKEN_LENGTH 256

// Helper function to remove leading and trailing whitespace.
void trim_whitespace(char* str) {
    if (str == NULL || *str == '\0') {
        return;
    }

    char* start = str;
    while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') {
        start++;
    }

    char* end = str + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }

    *(end + 1) = '\0';
    memmove(str, start, strlen(start) + 1);
}

// Check if a token is a valid command name (not an operator).
bool is_command_or_arg(const char* token) {
    if (token == NULL || strlen(token) == 0) return false;
    return (strpbrk(token, "|&><;") == NULL);
}

// Check if a token is one of the valid operators.
bool is_valid_operator(const char* token) {
    return (strcmp(token, "|") == 0 || strcmp(token, ";") == 0 ||
            strcmp(token, "<") == 0 || strcmp(token, ">") == 0 ||
            strcmp(token, ">>") == 0 || strcmp(token, "&") == 0);
}

// A robust tokenizer that correctly separates operators and arguments by copying them.
void tokenize_input(char* line, char* tokens[], int* token_count) {
    *token_count = 0;
    char* current = line;

    while (*current != '\0' && *token_count < MAX_TOKENS) {
        // Skip leading whitespace.
        while (*current == ' ' || *current == '\t' || *current == '\n' || *current == '\r') {
            current++;
        }
        if (*current == '\0') break;

        char token_buffer[MAX_TOKEN_LENGTH];
        int buffer_index = 0;
        
        // Handle two-character operators first to prevent issues with single-character operators.
        if (*current == '>' && current[1] == '>') {
            tokens[(*token_count)++] = strdup(">>");
            current += 2;
            continue;
        }

        // Handle one-character operators.
        if (strchr("|&><;", *current) != NULL) {
            token_buffer[buffer_index++] = *current;
            token_buffer[buffer_index] = '\0';
            tokens[(*token_count)++] = strdup(token_buffer);
            current++;
            continue;
        }
        
        // NEW: Handle quoted strings.
        if (*current == '"') {
            current++; // Move past the opening quote
            char* token_start = current;
            while (*current != '\0' && *current != '"') {
                current++;
            }
            int len = current - token_start;
            strncpy(token_buffer, token_start, len);
            token_buffer[len] = '\0';
            tokens[(*token_count)++] = strdup(token_buffer);
            if (*current == '"') {
                current++; // Move past the closing quote
            }
            continue;
        }

        // Otherwise, it's a word (command or arg).
        char* token_start = current;
        while (*current != '\0' && !strchr(" |&><;", *current) &&
               *current != '\t' && *current != '\n' && *current != '\r') {
            current++;
        }
        int len = current - token_start;
        strncpy(token_buffer, token_start, len);
        token_buffer[len] = '\0';
        tokens[(*token_count)++] = strdup(token_buffer);
    }
}

// This function validates a single atomic command group (between | or ;).
bool validate_atomic_command(char* tokens[], int start_idx, int end_idx) {
    if (start_idx > end_idx) {
        return false;
    }
    
    // An atomic command must start with a valid command name.
    if (!is_command_or_arg(tokens[start_idx])) {
        return false;
    }

    // Iterate through the tokens in the atomic command to check for invalid sequences.
    for (int i = start_idx; i <= end_idx; i++) {
        char* current_token = tokens[i];
        
        // Redirection operators must be followed by a command/arg (a name).
        if (strcmp(current_token, "<") == 0 || strcmp(current_token, ">") == 0 || strcmp(current_token, ">>") == 0) {
            // Check if the next token is a valid name.
            if (i + 1 > end_idx || !is_command_or_arg(tokens[i+1])) {
                return false;
            }
        }
    }

    return true;
}

// // The main parsing function.
// bool parse_input(char* line) {
//     if (line == NULL) return false;

//     char* line_copy = strdup(line);
//     if (!line_copy) {
//         perror("strdup");
//         return false;
//     }

//     trim_whitespace(line_copy);
//     if (strlen(line_copy) == 0) {
//         free(line_copy);
//         return true;
//     }

//     char* tokens[MAX_TOKENS];
//     int token_count = 0;
//     tokenize_input(line_copy, tokens, &token_count);

//     if (token_count == 0) {
//         free(line_copy);
//         return true;
//     }

//     int current_cmd_start = 0;
    
//     // The main loop that iterates over command groups.
//     for (int i = 0; i < token_count; i++) {
//         char* current_token = tokens[i];

//         if (strcmp(current_token, ";") == 0 || strcmp(current_token, "|") == 0) {
//             // Validate the command group before the operator.
//             if (!validate_atomic_command(tokens, current_cmd_start, i - 1)) {
//                 free(line_copy);
//                 return false;
//             }
//             // An operator cannot be at the end of the input (checked later)
//             // or followed by another operator.
//             if (i + 1 >= token_count || is_valid_operator(tokens[i+1])) {
//                 free(line_copy);
//                 return false;
//             }
//             current_cmd_start = i + 1; // Start of the next command group.
//         } else if (strcmp(current_token, "&") == 0) {
//             if (i != token_count - 1) { // & must be the very last token
//                 free(line_copy);
//                 return false;
//             }
//         } else {
//             // Every token must be valid.
//             if (!is_command_or_arg(current_token) && !is_valid_operator(current_token)) {
//                  free(line_copy);
//                  return false;
//             }
//         }
//     }
    
//     // Validate the final command group.
//     if (!validate_atomic_command(tokens, current_cmd_start, token_count - 1)) {
//         free(line_copy);
//         return false;
//     }

//     free(line_copy);
//     return true;
// }


// The main parsing function.
bool parse_input(char* line) {
    if (line == NULL) return false;

    char* line_copy = strdup(line);
    if (!line_copy) {
        perror("strdup");
        return false;
    }

    trim_whitespace(line_copy);
    if (strlen(line_copy) == 0) {
        free(line_copy);
        return true;
    }

    char* tokens[MAX_TOKENS];
    int token_count = 0;
    tokenize_input(line_copy, tokens, &token_count);

    if (token_count == 0) {
        free(line_copy);
        return true;
    }

    int current_cmd_start = 0;

    for (int i = 0; i < token_count; i++) {
        char* current_token = tokens[i];

        if (strcmp(current_token, "|") == 0) {
            // Validate the group before the pipe
            if (!validate_atomic_command(tokens, current_cmd_start, i - 1)) {
                free(line_copy);
                return false;
            }
            // Pipe cannot be at start or end or adjacent to another operator
            if (i + 1 >= token_count || is_valid_operator(tokens[i+1])) {
                free(line_copy);
                return false;
            }
            current_cmd_start = i + 1;
        } else if (strcmp(current_token, ";") == 0 || strcmp(current_token, "&") == 0) {
            // Validate the group before the separator
            if (!validate_atomic_command(tokens, current_cmd_start, i - 1)) {
                free(line_copy);
                return false;
            }
            // Allow separator at end (e.g., trailing ';' or '&')
            if (i == token_count - 1) {
                current_cmd_start = i + 1; // nothing follows
                break;
            }
            // Next token must start a new command group (not operator or pipe)
            if (is_valid_operator(tokens[i+1]) || strcmp(tokens[i+1], "|") == 0) {
                free(line_copy);
                return false;
            }
            current_cmd_start = i + 1;
        } else {
            // Every token must be either a word or a valid operator
            if (!is_command_or_arg(current_token) && !is_valid_operator(current_token)) {
                free(line_copy);
                return false;
            }
        }
    }

    // Validate the final command group if any tokens remain after the last separator
    if (current_cmd_start < token_count) {
        if (!validate_atomic_command(tokens, current_cmd_start, token_count - 1)) {
            free(line_copy);
            return false;
        }
    }

    free(line_copy);
    return true;
}