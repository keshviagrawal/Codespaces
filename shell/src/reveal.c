// #define _POSIX_C_SOURCE 200809L
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <stdbool.h>
// #include <dirent.h>
// #include <unistd.h>
// #include "../include/reveal.h"
// #include "../include/hop.h" // For the change_directory logic

// // Comparison function for qsort to sort strings lexicographically.
// int compare_strings(const void* a, const void* b) {
//     const char* str_a = *(const char**)a;
//     const char* str_b = *(const char**)b;
//     return strcmp(str_a, str_b);
// }

// /**
//  * @brief Gets the absolute path of the directory to reveal.
//  * @param path_arg The path argument passed to reveal.
//  * @param prev_dir The previous directory from the shell's state.
//  * @param home_dir The home directory from the shell's state.
//  * @param target_path_buffer A buffer to store the resulting absolute path.
//  * @return Returns true on success, false on failure (e.g., path not found).
//  */
// bool get_reveal_path(const char* path_arg, char* prev_dir, const char* home_dir, char* target_path_buffer) {
//     if (path_arg == NULL || strcmp(path_arg, "~") == 0) {
//         if (home_dir == NULL) {
//             fprintf(stderr, "Home directory not set.\n");
//             return false;
//         }
//         strcpy(target_path_buffer, home_dir);
//     } else if (strcmp(path_arg, ".") == 0) {
//         if (getcwd(target_path_buffer, 1024) == NULL) {
//             perror("getcwd failed");
//             return false;
//         }
//     } else if (strcmp(path_arg, "..") == 0) {
//         if (getcwd(target_path_buffer, 1024) == NULL) {
//             perror("getcwd failed");
//             return false;
//         }
//         char* last_slash = strrchr(target_path_buffer, '/');
//         if (last_slash != NULL && strcmp(target_path_buffer, "/") != 0) {
//             *last_slash = '\0';
//         } else if (strcmp(target_path_buffer, "/") != 0) {
//             strcpy(target_path_buffer, "/");
//         }
//     } else if (strcmp(path_arg, "-") == 0) {
//         if (prev_dir == NULL) {
//             fprintf(stderr, "No such directory!\n");
//             return false;
//         }
//         strcpy(target_path_buffer, prev_dir);
//     } else {
//         // Assume it's a regular path
//         strcpy(target_path_buffer, path_arg);
//     }

//     // Check if the directory exists
//     DIR* dir = opendir(target_path_buffer);
//     if (dir == NULL) {
//         return false;
//     }
//     closedir(dir);
//     return true;
// }

// bool reveal(char** args, int num_args, char** prev_dir, const char* home_dir) {
//     bool show_all = false;
//     bool line_by_line = false;
//     const char* path_arg = NULL;

//     // First, check for too many arguments as per the requirement
//     int non_flag_count = 0;
//     for (int i = 0; i < num_args; i++) {
//         if (args[i][0] != '-') {
//             non_flag_count++;
//         }
//     }
//     if (non_flag_count > 1) {
//         fprintf(stderr, "reveal: Invalid Syntax!\n");
//         return false;
//     }

//     // Parse flags and identify the path argument
//     for (int i = 0; i < num_args; i++) {
//         if (args[i][0] == '-') {
//             for (size_t j = 1; j < strlen(args[i]); j++) {
//                 if (args[i][j] == 'a') {
//                     show_all = true;
//                 } else if (args[i][j] == 'l') {
//                     line_by_line = true;
//                 }
//             }
//         } else {
//             path_arg = args[i];
//         }
//     }

//     char target_path[1024];
//     if (path_arg == NULL) {
//         if (getcwd(target_path, sizeof(target_path)) == NULL) {
//             perror("getcwd failed");
//             return false;
//         }
//     } else {
//         if (!get_reveal_path(path_arg, *prev_dir, home_dir, target_path)) {
//             fprintf(stderr, "No such directory!\n");
//             return false;
//         }
//     }

//     DIR* dir = opendir(target_path);
//     if (dir == NULL) {
//         fprintf(stderr, "No such directory!\n");
//         return false;
//     }

//     struct dirent* entry;
//     char** filenames = NULL;
//     int file_count = 0;
//     int max_files = 10;

//     filenames = (char**)malloc(max_files * sizeof(char*));
//     if (filenames == NULL) {
//         perror("malloc failed");
//         closedir(dir);
//         return false;
//     }

//     while ((entry = readdir(dir)) != NULL) {
//         // Skip hidden files if -a is not set
//         if (!show_all && entry->d_name[0] == '.') {
//             continue;
//         }

//         // Dynamically grow the array if needed
//         if (file_count >= max_files) {
//             max_files *= 2;
//             filenames = (char**)realloc(filenames, max_files * sizeof(char*));
//             if (filenames == NULL) {
//                 perror("realloc failed");
//                 closedir(dir);
//                 return false;
//             }
//         }
//         filenames[file_count++] = strdup(entry->d_name);
//     }
//     closedir(dir);

//     // Sort the filenames
//     qsort(filenames, file_count, sizeof(char*), compare_strings);

//     // Print the filenames based on flags
//     for (int i = 0; i < file_count; i++) {
//         printf("%s%s", filenames[i], (line_by_line) ? "\n" : (i == file_count - 1) ? "" : " ");
//         free(filenames[i]); // Free memory for each string
//     }
//     printf("\n"); // Ensure a newline at the end

//     free(filenames);
//     return true;
// }



// #define _POSIX_C_SOURCE 200809L
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <stdbool.h>
// #include <dirent.h>
// #include <unistd.h>
// #include "../include/reveal.h"
// #include "../include/hop.h" // For the change_directory logic

// // Comparison function for qsort to sort strings lexicographically.
// int compare_strings(const void* a, const void* b) {
//     const char* str_a = *(const char**)a;
//     const char* str_b = *(const char**)b;
//     return strcmp(str_a, str_b);
// }

// /**
//  * @brief Gets the absolute path of the directory to reveal.
//  * @param path_arg The path argument passed to reveal.
//  * @param prev_dir The previous directory from the shell's state.
//  * @param home_dir The home directory from the shell's state.
//  * @param target_path_buffer A buffer to store the resulting absolute path.
//  * @return Returns true on success, false on failure (e.g., path not found).
//  */
// bool get_reveal_path(const char* path_arg, char* prev_dir, const char* home_dir, char* target_path_buffer) {
//     if (path_arg == NULL || strcmp(path_arg, "~") == 0) {
//         if (home_dir == NULL) {
//             fprintf(stderr, "Home directory not set.\n");
//             return false;
//         }
//         strcpy(target_path_buffer, home_dir);
//     } else if (strcmp(path_arg, ".") == 0) {
//         if (getcwd(target_path_buffer, 1024) == NULL) {
//             perror("getcwd failed");
//             return false;
//         }
//     } else if (strcmp(path_arg, "..") == 0) {
//         if (getcwd(target_path_buffer, 1024) == NULL) {
//             perror("getcwd failed");
//             return false;
//         }
//         char* last_slash = strrchr(target_path_buffer, '/');
//         if (last_slash != NULL && strcmp(target_path_buffer, "/") != 0) {
//             *last_slash = '\0';
//         } else if (strcmp(target_path_buffer, "/") != 0) {
//             strcpy(target_path_buffer, "/");
//         }
//     } else if (strcmp(path_arg, "-") == 0) {
//         if (prev_dir == NULL) {
//             fprintf(stderr, "No such directory!\n");
//             return false;
//         }
//         strcpy(target_path_buffer, prev_dir);
//     } else {
//         // Assume it's a regular path
//         strcpy(target_path_buffer, path_arg);
//     }

//     // Check if the directory exists
//     DIR* dir = opendir(target_path_buffer);
//     if (dir == NULL) {
//         return false;
//     }
//     closedir(dir);
//     return true;
// }

// bool reveal(char** args, int num_args, char** prev_dir, const char* home_dir) {
//     bool show_all = false;
//     bool line_by_line = false;
//     const char* path_arg = NULL;

//     // First, check for too many arguments as per the requirement
//     int non_flag_count = 0;
//     for (int i = 0; i < num_args; i++) {
//         if (args[i][0] != '-') {
//             non_flag_count++;
//         }
//     }
//     if (non_flag_count > 2) { // Changed to > 2 because 'reveal' and one path are allowed
//         fprintf(stderr, "reveal: Invalid Syntax!\n");
//         return false;
//     }

//     // Parse flags and identify the path argument
//     // Start at index 1 to skip the command name itself
//     for (int i = 1; i < num_args; i++) {
//         if (args[i][0] == '-') {
//             for (size_t j = 1; j < strlen(args[i]); j++) {
//                 if (args[i][j] == 'a') {
//                     show_all = true;
//                 } else if (args[i][j] == 'l') {
//                     line_by_line = true;
//                 }
//             }
//         } else {
//             path_arg = args[i];
//         }
//     }

//     char target_path[1024];
//     if (path_arg == NULL) {
//         if (getcwd(target_path, sizeof(target_path)) == NULL) {
//             perror("getcwd failed");
//             return false;
//         }
//     } else {
//         if (!get_reveal_path(path_arg, *prev_dir, home_dir, target_path)) {
//             fprintf(stderr, "No such directory!\n");
//             return false;
//         }
//     }

//     DIR* dir = opendir(target_path);
//     if (dir == NULL) {
//         fprintf(stderr, "No such directory!\n");
//         return false;
//     }

//     struct dirent* entry;
//     char** filenames = NULL;
//     int file_count = 0;
//     int max_files = 10;

//     filenames = (char**)malloc(max_files * sizeof(char*));
//     if (filenames == NULL) {
//         perror("malloc failed");
//         closedir(dir);
//         return false;
//     }

//     while ((entry = readdir(dir)) != NULL) {
//         // Skip hidden files if -a is not set
//         if (!show_all && entry->d_name[0] == '.') {
//             continue;
//         }

//         // Dynamically grow the array if needed
//         if (file_count >= max_files) {
//             max_files *= 2;
//             filenames = (char**)realloc(filenames, max_files * sizeof(char*));
//             if (filenames == NULL) {
//                 perror("realloc failed");
//                 closedir(dir);
//                 return false;
//             }
//         }
//         filenames[file_count++] = strdup(entry->d_name);
//     }
//     closedir(dir);

//     // Sort the filenames
//     qsort(filenames, file_count, sizeof(char*), compare_strings);

//     // Print the filenames based on flags
//     for (int i = 0; i < file_count; i++) {
//         printf("%s%s", filenames[i], (line_by_line) ? "\n" : (i == file_count - 1) ? "" : " ");
//         free(filenames[i]); // Free memory for each string
//     }
//     printf("\n"); // Ensure a newline at the end

//     free(filenames);
//     return true;
// }




// #define _POSIX_C_SOURCE 200809L
// #include <stdio.h>
// #include <stdlib.h>
// #include <limits.h>
// #include <string.h>
// #include <stdbool.h>
// #include <dirent.h>
// #include <sys/stat.h>
// #include <libgen.h>
// #include <unistd.h>
// #include <strings.h> // Required for strcasecmp
// #include "../include/reveal.h"
// #include "../include/hop.h" // For the change_directory logic

// // Comparison function for qsort to sort strings lexicographically.
// // This function now uses strcasecmp for a case-insensitive sort, as required by the tests.
// int compare_strings(const void* a, const void* b) {
//     const char* str_a = *(const char**)a;
//     const char* str_b = *(const char**)b;
//     return strcasecmp(str_a, str_b);
// }


// bool get_reveal_path(const char* path_arg, char* prev_dir, const char* home_dir, char* target_path_buffer) {
//     if (path_arg == NULL || strcmp(path_arg, "~") == 0) {
//         if (home_dir == NULL) {
//             fprintf(stderr, "Home directory not set.\n");
//             return false;
//         }
//         strcpy(target_path_buffer, home_dir);
//     } else if (strcmp(path_arg, ".") == 0) {
//         if (getcwd(target_path_buffer, 1024) == NULL) {
//             perror("getcwd failed");
//             return false;
//         }
//     } else if (strcmp(path_arg, "..") == 0) {
//         if (getcwd(target_path_buffer, 1024) == NULL) {
//             perror("getcwd failed");
//             return false;
//         }
//         char* last_slash = strrchr(target_path_buffer, '/');
//         if (last_slash != NULL && strcmp(target_path_buffer, "/") != 0) {
//             *last_slash = '\0';
//         } else if (strcmp(target_path_buffer, "/") != 0) {
//             strcpy(target_path_buffer, "/");
//         }
//     } else if (strcmp(path_arg, "-") == 0) {
//         if (prev_dir == NULL) {
//             // fprintf(stderr, "No such directory!\n");
//             return false;
//         }
//         //printf("%s\n", prev_dir);
//         strcpy(target_path_buffer, prev_dir);
//     }
    
//     // else {
//     //     strcpy(target_path_buffer, path_arg);
//     // }

//      else {
//         // Use realpath to handle relative paths correctly
//         if (realpath(path_arg, target_path_buffer) == NULL) {
//             // realpath returns NULL on error, e.g., if the file/dir doesn't exist
//             return false;
//         }
//     }


//     struct stat st;
//     if (stat(target_path_buffer, &st) == -1) {
//         return false;
//     }
//     if (!S_ISDIR(st.st_mode)) {
//         return false;
//     }
    
//     return true;

//     // // Check if the directory exists
//     // DIR* dir = opendir(target_path_buffer);
//     // if (dir == NULL) {
//     //     return false;
//     // }
//     // closedir(dir);
//     // return true;
// }


// bool reveal(char** args, int num_args, char** prev_dir, const char* home_dir) {
//     bool show_all = false;
//     bool line_by_line = false;
//     const char* path_arg = NULL;


//     int non_flag_count = 0;
//     for (int i = 0; i < num_args; i++) {
//         if (strcmp(args[i], "-") == 0 || args[i][0] != '-') {
//             non_flag_count++;
//         }
//     }

//     if (non_flag_count > 1) {
//         fprintf(stderr, "reveal: Invalid Syntax!\n");
//         return false;
//     }


//     for (int i = 0; i < num_args; i++) {
//         if (strcmp(args[i], "-") == 0) {
//             // Treat single dash as previous-directory path argument
//             path_arg = args[i];
//         } else if (args[i][0] == '-') {
//             // Parse flags like -a, -l, -al
//             for (size_t j = 1; j < strlen(args[i]); j++) {
//                 if (args[i][j] == 'a') {
//                     show_all = true;
//                 } else if (args[i][j] == 'l') {
//                     line_by_line = true;
//                 }
//             }
//         } else {
//             path_arg = args[i];
//         }
//     }

//     // char target_path[1024];
//     // if (path_arg == NULL) {
//     //     if (getcwd(target_path, sizeof(target_path)) == NULL) {
//     //         perror("getcwd failed");
//     //         return false;
//     //     }
//     // } else {
//     //     if (!get_reveal_path(path_arg, *prev_dir, home_dir, target_path)) {
//     //         fprintf(stderr, "No such directory!\n");
//     //         return false;
//     //     }
//     // }

//     // DIR* dir = opendir(target_path);
//     // if (dir == NULL) {
//     //     fprintf(stderr, "No such directory!\n");
//     //     return false;
//     // }

//     char target_path[1024];
//     const char* dir_to_open = NULL;

//     if (path_arg == NULL) {
//         // No path argument given, default to the current directory
//         if (getcwd(target_path, sizeof(target_path)) == NULL) {
//             perror("getcwd failed");
//             return false;
//         }
//         dir_to_open = target_path;
//     } else {
//         // Resolve the provided path argument
//         char resolved_path[1024];
//         if (realpath(path_arg, resolved_path) == NULL) {
//             fprintf(stderr, "No such directory!\n");
//             return false;
//         }
//         dir_to_open = resolved_path;
//     }

//     DIR* dir = opendir(dir_to_open);
//     if (dir == NULL) {
//         fprintf(stderr, "No such directory!\n");
//         return false;
//     }

//     struct dirent* entry;
//     char** filenames = NULL;
//     int file_count = 0;
//     int max_files = 10;

//     filenames = (char**)malloc(max_files * sizeof(char*));
//     if (filenames == NULL) {
//         perror("malloc failed");
//         closedir(dir);
//         return false;
//     }

//     while ((entry = readdir(dir)) != NULL) {
//         // Skip hidden files if -a is not set
//         if (!show_all && entry->d_name[0] == '.') {
//             continue;
//         }

//         // Dynamically grow the array if needed
//         if (file_count >= max_files) {
//             max_files *= 2;
//             filenames = (char**)realloc(filenames, max_files * sizeof(char*));
//             if (filenames == NULL) {
//                 perror("realloc failed");
//                 closedir(dir);
//                 return false;
//             }
//         }
//         filenames[file_count++] = strdup(entry->d_name);
//     }
//     closedir(dir);

//     // Sort the filenames
//     qsort(filenames, file_count, sizeof(char*), compare_strings);

//     // Print the filenames based on flags
//     for (int i = 0; i < file_count; i++) {
//         printf("%s%s", filenames[i], (line_by_line) ? "\n" : (i == file_count - 1) ? "" : " ");
//         free(filenames[i]); // Free memory for each string
//     }
//     printf("\n"); // Ensure a newline at the end

//     free(filenames);
//     return true;
// }



#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <libgen.h>
#include <unistd.h>
#include <strings.h> // Required for strcasecmp
#include "../include/reveal.h"
#include "../include/hop.h" // For the change_directory logic

// Comparison function for qsort to sort strings lexicographically.
// This function now uses strcasecmp for a case-insensitive sort, as required by the tests.
int compare_strings(const void* a, const void* b) {
    const char* str_a = *(const char**)a;
    const char* str_b = *(const char**)b;
    return strcasecmp(str_a, str_b);
}


bool get_reveal_path(const char* path_arg, char* prev_dir, const char* home_dir, char* target_path_buffer) {
    if (path_arg == NULL || strcmp(path_arg, "~") == 0) {
        if (home_dir == NULL) {
            fprintf(stderr, "Home directory not set.\n");
            return false;
        }
        strcpy(target_path_buffer, home_dir);
    } else if (strcmp(path_arg, ".") == 0) {
        if (getcwd(target_path_buffer, 1024) == NULL) {
            perror("getcwd failed");
            return false;
        }
    } else if (strcmp(path_arg, "..") == 0) {
        if (getcwd(target_path_buffer, 1024) == NULL) {
            perror("getcwd failed");
            return false;
        }
        char* last_slash = strrchr(target_path_buffer, '/');
        if (last_slash != NULL && strcmp(target_path_buffer, "/") != 0) {
            *last_slash = '\0';
        } else if (strcmp(target_path_buffer, "/") != 0) {
            strcpy(target_path_buffer, "/");
        }
    } else if (strcmp(path_arg, "-") == 0) {
        if (prev_dir == NULL) {
            // fprintf(stderr, "No such directory!\n");
            return false;
        }
        //printf("%s\n", prev_dir);
        strcpy(target_path_buffer, prev_dir);
    } else {
        
        strcpy(target_path_buffer, path_arg);
        
    }

    // // Check if the directory exists
    // DIR* dir = opendir(target_path_buffer);
    // if (dir == NULL) {
    //     return false;
    // }
    // closedir(dir);
    return true;
}


bool reveal(char** args, int num_args, char** prev_dir, const char* home_dir) {
    bool show_all = false;
    bool line_by_line = false;
    const char* path_arg = NULL;


    int non_flag_count = 0;
    for (int i = 0; i < num_args; i++) {
        if (strcmp(args[i], "-") == 0 || args[i][0] != '-') {
            non_flag_count++;
        }
    }

    if (non_flag_count > 1) {
        fprintf(stderr, "reveal: Invalid Syntax!\n");
        return false;
    }


    for (int i = 0; i < num_args; i++) {
        if (strcmp(args[i], "-") == 0) {
            // Treat single dash as previous-directory path argument
            path_arg = args[i];
        } else if (args[i][0] == '-') {
            // Parse flags like -a, -l, -al
            for (size_t j = 1; j < strlen(args[i]); j++) {
                if (args[i][j] == 'a') {
                    show_all = true;
                } else if (args[i][j] == 'l') {
                    line_by_line = true;
                }
            }
        } else {
            path_arg = args[i];
        }
    }

    char target_path[1024];
    if (path_arg == NULL) {
        if (getcwd(target_path, sizeof(target_path)) == NULL) {
            perror("getcwd failed");
            return false;
        }
    } else {
        if (!get_reveal_path(path_arg, *prev_dir, home_dir, target_path)) {
            fprintf(stderr, "No such directory!\n");
            return false;
        }
    }

    DIR* dir = opendir(target_path);
    if (dir == NULL) {
        fprintf(stderr, "No such directory!\n");
        return false;
    }

    struct dirent* entry;
    char** filenames = NULL;
    int file_count = 0;
    int max_files = 10;

    filenames = (char**)malloc(max_files * sizeof(char*));
    if (filenames == NULL) {
        perror("malloc failed");
        closedir(dir);
        return false;
    }

    while ((entry = readdir(dir)) != NULL) {
        // Skip hidden files if -a is not set
        if (!show_all && entry->d_name[0] == '.') {
            continue;
        }

        // Dynamically grow the array if needed
        if (file_count >= max_files) {
            max_files *= 2;
            filenames = (char**)realloc(filenames, max_files * sizeof(char*));
            if (filenames == NULL) {
                perror("realloc failed");
                closedir(dir);
                return false;
            }
        }
        filenames[file_count++] = strdup(entry->d_name);
    }
    closedir(dir);

    // Sort the filenames
    qsort(filenames, file_count, sizeof(char*), compare_strings);

    // Print the filenames based on flags
    for (int i = 0; i < file_count; i++) {
        printf("%s%s", filenames[i], (line_by_line) ? "\n" : (i == file_count - 1) ? "" : " ");
        free(filenames[i]); // Free memory for each string
    }
    printf("\n"); // Ensure a newline at the end

    free(filenames);
    return true;
}