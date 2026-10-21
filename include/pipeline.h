/*
 * Copyright (C) 2026 volatilespectre
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PIPELINE_H_
#define PIPELINE_H_

#include <sys/types.h>
#include <time.h>

int run_pipeline(char ***pipeline, int command_count, pid_t *pids,
                 struct timespec *starts, int output_fd, int errors_fd);

#endif
