#ifndef OUTPUT_H_
#define OUTPUT_H_

#include <sys/types.h>

int show_output(char ***pipeline, int command_count, pid_t *pids,
                struct timespec *starts);

#endif