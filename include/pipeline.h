#ifndef PIPELINE_H_
#define PIPELINE_H_

#include <sys/types.h>

int run_pipeline(char ***pipeline, int command_count, pid_t *pids);

#endif