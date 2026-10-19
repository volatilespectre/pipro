/*
 * Copyright (C) 2026 volatilespectre
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <ctype.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int parse_pipeline(int argc, char *argv[], char ****pipeline,
                   int *command_count) {
  if (argc == 0) {
    printf("You haven't entered a command to be profiled. Use --help for usage "
           "information. ");
    return EXIT_FAILURE;
  }

  // Start dynamic list
  int count = 0;
  int capacity = 5;
  char **tokens = malloc(capacity * sizeof(char *));
  if (tokens == NULL) {
    perror("malloc");
    return EXIT_FAILURE;
  }
  char *current = argv[0];

  while (*current != '\0') {
    // Skip leading spaces
    while (isspace(*current)) {
      current++;
    }

    // Ensure no empty token
    if (*current == '\0') {
      break;
    }

    // Tokens only break on spaces, so skip all non space characters
    char *start = current;
    while (*current != '\0' && !isspace(*current)) {
      current++;
    }

    // Add letters to the next token
    int length = current - start;
    tokens[count] = malloc((length + 1) * sizeof(char));
    memcpy(tokens[count], start, length);
    tokens[count][length] = '\0';
    count++;
    if (count == capacity) {
      capacity += 5;
      char **tmp = realloc(tokens, capacity * sizeof(char *));
      if (tmp == NULL) {
        perror("realloc");
        return EXIT_FAILURE;
      } else {
        tokens = tmp;
      }
    }
  }

  // Count the number of pipes and commands
  *command_count = 1;
  for (int i = 0; i < count; i++) {
    if (strcmp(tokens[i], "|") == 0) {
      *command_count += 1;
    }
  }

  *pipeline = malloc(*command_count * sizeof(char **));
  if (*pipeline == NULL) {
    perror("malloc");
    return EXIT_FAILURE;
  }

  int start = 0;
  int len = 0;
  int command_number = 0;
  for (int i = 0; i <= count; i++) {
    if (i != count && strcmp(tokens[i], "|") != 0) {
      continue;
    }

    len = i - start;
    (*pipeline)[command_number] = malloc((len + 1) * sizeof(char *));
    if ((*pipeline)[command_number] == NULL) {
      perror("malloc");
      return EXIT_FAILURE;
    }
    for (int j = 0; j < len; j++) {
      (*pipeline)[command_number][j] = tokens[start + j];
    }
    (*pipeline)[command_number][len] = NULL;
    if (i != count) {
      free(tokens[i]);
    }
    start = i + 1;
    command_number++;
  }

  free(tokens);

  return EXIT_SUCCESS;
}

int parse_args(int argc, char **argv, char ****pipeline, int *command_count,
               bool *is_static, bool *help, bool *quiet, int *decimal_places) {
  int c;

  char *short_options = "shqd:";

  // Map long options to their short equivalents
  const struct option long_options[] = {
      {"help", no_argument, NULL, 'h'},
      {"static", no_argument, NULL, 's'},
      {"quiet", no_argument, NULL, 'q'},
      {"decimals", required_argument, NULL, 'd'},
      {NULL, 0, NULL, 0}};

  // Parse options in a loop
  while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) !=
         -1) {
    switch (c) {
    case 'h':
      *help = true;
      break;
    case 's':
      *is_static = true;
      break;
    case 'q':
      *quiet = true;
      break;
    case 'd': {
      char *end;
      *decimal_places = (int)strtol(optarg, &end, 10);
      if (end == optarg || *end != '\0' || *decimal_places < 0 ||
          *decimal_places > 9) {
        *help = true;
        return EXIT_FAILURE;
      }
      break;
    }
    case '?':
      // getopt_long automatically prints an error message
      return EXIT_FAILURE;
    default:
      break;
    }
  }

  if (*help) {
    return EXIT_SUCCESS;
  }
  if (optind == argc) {
    printf("You haven't entered a command to be profiled. Use --help for usage "
           "information. \n");
    return EXIT_FAILURE;
  }

  return parse_pipeline(argc - optind, &argv[optind], pipeline, command_count);
}