#ifndef OUTPUT_H_
#define OUTPUT_H_

#include <sys/types.h>

#include <stdbool.h>
#include <stdio.h>

int show_output(char ***pipeline, int command_count, pid_t *pids,
                struct timespec *start, bool is_static, int decimal_places, FILE *output, bool quiet);

#endif