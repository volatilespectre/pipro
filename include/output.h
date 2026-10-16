/*
 * Copyright (C) 2026 volatilespectre
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef OUTPUT_H_
#define OUTPUT_H_

#include <sys/types.h>

#include <stdbool.h>
#include <stdio.h>

int show_output(char ***pipeline, int command_count, pid_t *pids,
                struct timespec *start, bool is_static, int decimal_places,
                bool quiet, FILE *output, FILE *errors);

void print_help();

#endif