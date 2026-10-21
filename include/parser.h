/*
 * Copyright (C) 2026 volatilespectre
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PARSER_H_
#define PARSER_H_

#include <stdbool.h>

int parse_args(int argc, char **argv, char ****pipeline, int *command_count,
               bool *is_static, bool *help, bool *quiet, int *decimal_places);

int parse_pipeline(int argc, char *argv[], char ****pipeline,
                   int *command_count);

#endif
