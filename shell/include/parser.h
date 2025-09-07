#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

bool parse_input(char *input);
void tokenize_input(char* line, char* tokens[], int* token_count);

#endif
