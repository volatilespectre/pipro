/*
 * Copyright (C) 2026 volatilespectre
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#if defined(__linux__)
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#endif

#include "parser.h"
#include "pipeline.h"
#include "time_utils.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Position is of leftmost character in 4-char block, can be between 1 and 37,
// direction is either right (true) or left (false)
struct status_bar_info {
  int position;
  bool direction;
};

// Sets up starting positions of status bars, will try to space apart until bar
// is full
void status_bar_setup(struct status_bar_info *info, int number,
                      int command_count) {
  info->direction = true;
  if (command_count <= 6) {
    info->position = number * 6 + 1;
    return;
  } else if (command_count <= 9) {
    info->position = number * 4 + 1;
    return;
  } else if (command_count <= 18) {
    info->position = number * 2 + 1;
    return;
  } else {
    info->position = number % 36 + 1;
    return;
  }
}

void generate_bar(struct status_bar_info *info, char *status_bar) {
  int i = 1;
  status_bar[0] = '[';
  if (info->position > 1) {
    for (; i < info->position; i++) {
      status_bar[i] = ' ';
    }
  }
  for (; i < (info->position) + 4; i++) {
    status_bar[i] = '#';
  }
  for (; i < 41; i++) {
    status_bar[i] = ' ';
  }
  status_bar[41] = ']';
  status_bar[42] = '\0';
  if (info->direction) {
    if (info->position == 37) {
      info->direction = false;
      info->position = 36;
    } else {
      info->position++;
    }
  } else {
    if (info->position == 1) {
      info->direction = true;
      info->position = 2;
    } else {
      info->position--;
    }
  }
}

void generate_progress_line(char *command_name, double runtime,
                            char *status_bar, char *buffer) {
  snprintf(buffer, 256, "Running: %-15s %-50s Elapsed: %f\n", command_name,
           status_bar, runtime);
}

void generate_completed_line(char *command_name, int status, double runtime,
                             struct rusage usage, char *buffer, bool colour,
                             int decimal_places) {
  double user_cpu = timeval_seconds(usage.ru_utime);
  double sys_cpu = timeval_seconds(usage.ru_stime);

  long peak_mem_units = usage.ru_maxrss;
  double peak_mem_mb;

#if defined(__APPLE__) && defined(__MACH__)
  // MacOS gives ru_maxrss in bytes
  peak_mem_mb = peak_mem_units / (1024.0 * 1024.0);
#else
  // Linux and BSDs give ru_maxrss in kilobytes
  peak_mem_mb = peak_mem_units / 1024.0;
#endif

  char status_str[35];
  int padding_width = 20;
  if (colour) {
    if (WIFEXITED(status)) {
      padding_width = 30;
      if (WEXITSTATUS(status) == 0) {
        snprintf(status_str, sizeof(status_str),
                 "Returned \033[32m%d (OK) \033[39m", WEXITSTATUS(status));
      } else {
        snprintf(status_str, sizeof(status_str),
                 "Returned \033[31m%d (Fail) \033[39m", WEXITSTATUS(status));
      }
    } else if (WIFSIGNALED(status)) {
      snprintf(status_str, sizeof(status_str), "Killed (Signal %d)",
               WTERMSIG(status));
    } else {
      snprintf(status_str, sizeof(status_str), "UNKNOWN");
    }
  } else {
    if (WIFEXITED(status)) {
      if (WEXITSTATUS(status) == 0) {
        snprintf(status_str, sizeof(status_str), "Returned %d (OK)",
                 WEXITSTATUS(status));
      } else {
        snprintf(status_str, sizeof(status_str), "Returned %d (Fail)",
                 WEXITSTATUS(status));
      }
    } else if (WIFSIGNALED(status)) {
      snprintf(status_str, sizeof(status_str), "Signal (%d)", WTERMSIG(status));
    } else {
      snprintf(status_str, sizeof(status_str), "UNKNOWN");
    }
  }
  snprintf(buffer, 256, "%-20s %-*s %-20.*f %-15.*f %-15.*f %-20.3f\n",
           command_name, padding_width, status_str, decimal_places, runtime,
           decimal_places, user_cpu, decimal_places, sys_cpu, peak_mem_mb);
}

int show_output(char ***pipeline, int command_count, pid_t *pids,
                struct timespec *starts, bool is_static, int decimal_places,
                bool quiet, FILE *output, FILE *errors) {

  // These variables are needed for both interactive and static output
  int *statuses = malloc(command_count * sizeof(int));
  if (statuses == NULL) {
    perror("malloc");
    return EXIT_FAILURE;
  }
  struct timespec *ends = malloc(command_count * sizeof(struct timespec));
  if (ends == NULL) {
    perror("malloc");
    return EXIT_FAILURE;
  }

  struct rusage usage;
  int status;

  if (isatty(STDOUT_FILENO) && !is_static) {
    // Variables only used for interactive output
    bool *completed = calloc(command_count, sizeof(bool));
    if (completed == NULL) {
      perror("calloc");
      return EXIT_FAILURE;
    }
    struct status_bar_info *status_bar_infos =
        malloc(command_count * sizeof(struct status_bar_info));
    if (status_bar_infos == NULL) {
      perror("malloc");
      return EXIT_FAILURE;
    }
    char (*lines)[256] = malloc(command_count * sizeof(*lines));
    if (lines == NULL) {
      perror("malloc");
      return EXIT_FAILURE;
    }
    for (int i = 0; i < command_count; i++) {
      lines[i][0] = '\0';
    }

    struct timespec last_redraw;
    struct timespec time;
    double elapsed;
    bool redraw = false;
    int completed_count = 0;

    int clock_return = clock_gettime(CLOCK_MONOTONIC, &last_redraw);
    if (clock_return == -1) {
      perror("clock_gettime");
      return EXIT_FAILURE;
    }

    // Initialise display
    printf("%-20s %-20s %-20s %-15s %-15s %-22s", "Command name", "Exit Status",
           "Wall-Clock Time (s)", "User CPU (s)", "System CPU (s)",
           "Peak memory usage (MB)\n");

    for (int i = 0; i < 117; i++) {
      putchar('-');
    }
    putchar('\n');

    // Initialise per-command display
    for (int i = 0; i < command_count; i++) {
      status_bar_setup(&status_bar_infos[i], i, command_count);
      char status_bar[43];
      generate_bar(&status_bar_infos[i], status_bar);
      printf("%-20s Running %-45s %.*fs\n", pipeline[i][0], status_bar,
             decimal_places, 0.000000000);
    }

    // Loop: Wait for 1/10^decimal_places s (e.g. 0.1s if decimal_places = 1),
    // then redraw terminal if needed (>0.1s) since last redraw, or process
    // finished)
    while (completed_count < command_count) {
      redraw = false;
      int finished_pid = wait4(-1, &status, WNOHANG, &usage);
      if (finished_pid == -1) {
        perror("wait4");
        return EXIT_FAILURE;
      } else if (finished_pid > 0) {
        for (int j = 0; j < command_count; j++) {
          if (pids[j] == finished_pid) {
            // Keep index and status, note that a redraw is needed
            statuses[j] = status;
            completed[j] = true;
            redraw = true;
            completed_count++;
            break;
          }
        }
      }
      // Calculate time since last
      int clock_return = clock_gettime(CLOCK_MONOTONIC, &time);
      if (clock_return == -1) {
        perror("clock_gettime");
        return EXIT_FAILURE;
      }

      timespec_subtract(&elapsed, &last_redraw, &time, decimal_places);
      if (elapsed >= 0.1) {
        redraw = true;
        last_redraw = time;
      }
      if (redraw) {
        printf("\033[%dA\r", command_count);
        for (int i = 0; i < command_count; i++) {
          double runtime;
          timespec_subtract(&runtime, &starts[i], &time, decimal_places);
          if (completed[i]) {
            if (lines[i][0] == '\0') {
              char line[256];
              generate_completed_line(pipeline[i][0], statuses[i], runtime,
                                      usage, line, true, decimal_places);
              snprintf(lines[i], sizeof(lines[i]), "%s", line);
            }
            printf("%s", lines[i]);
          } else {
            char status_bar[43];
            generate_bar(&status_bar_infos[i], status_bar);
            char line[256];
            generate_progress_line(pipeline[i][0], runtime, status_bar, line);
            printf("%s", line);
          }
        }
      }
      struct timespec delay;
      delay.tv_sec = 0;
      delay.tv_nsec = pow(10, (9 - decimal_places));
      if (nanosleep(&delay, NULL) == -1) {
        perror("nanosleep");
        return EXIT_FAILURE;
      }
    }

    free(completed);
    free(status_bar_infos);
    free(lines);
  } else {
    printf("%-20s %-20s %-20s %-15s %-15s %-22s", "Command name", "Exit Status",
           "Wall-Clock Time (s)", "User CPU (s)", "System CPU (s)",
           "Peak memory usage (MB)\n");

    for (int i = 0; i < 117; i++) {
      putchar('-');
    }
    putchar('\n');
    for (int i = 0; i < command_count; i++) {
      // Wait for the first command to finish, and get resource usage stats

      pid_t finished_pid = wait4(-1, &status, 0, &usage);
      int number = -1;

      if (finished_pid > 0) {
        for (int j = 0; j < command_count; j++) {
          if (pids[j] == finished_pid) {
            // 'i' is the index of the finished process.
            statuses[j] = status;
            // Keep that index
            number = j;
            break;
          }
        }

        // Check if PID not found
        if (number == -1) {
          fprintf(stderr, "Error: Unknown process reaped \n");
          return EXIT_FAILURE;
        }
      } else {
        perror("wait4");
        return EXIT_FAILURE;
      }

      // Calculate the time it took to run
      int clock_return = clock_gettime(CLOCK_MONOTONIC, &ends[number]);
      if (clock_return == -1) {
        perror("clock_gettime");
        return EXIT_FAILURE;
      }

      double runtime;
      timespec_subtract(&runtime, &starts[number], &ends[number], 3);

      char line[256];
      generate_completed_line(pipeline[number][0], statuses[number], runtime,
                              usage, line, false, decimal_places);
      printf("%s", line);
    }
  }

  if (fseek(errors, 0, SEEK_SET) != 0) {
    perror("fseek");
    return EXIT_FAILURE;
  }
  int c;
  if ((c = fgetc(errors)) != EOF) {
    printf("---------- ERRORS ----------\n");
    putchar(c);
    while ((c = fgetc(errors)) != EOF) {
      putchar(c);
    }
  }
  if (!quiet) {
    if (fseek(output, 0, SEEK_SET) != 0) {
      perror("fseek");
      return EXIT_FAILURE;
    }

    if ((c = fgetc(output)) != EOF) {
      printf("---------- OUTPUT ----------\n");
      putchar(c);
      while ((c = fgetc(output)) != EOF) {
        putchar(c);
      }
    }
  }

  free(statuses);
  free(ends);

  return EXIT_SUCCESS;
}

void print_help(void) {
  puts(
      "Usage: pipro [OPTIONS] \"COMMAND | COMMAND ...\"\n"
      "\n"
      "Profile a command pipeline and report resource usage for each command.\n"
      "\n"
      "Options:\n"
      "  -h, --help          Display this help message and exit\n"
      "  -s, --static        Disable the interactive progress display and "
      "remove ANSI escape sequences\n"
      "  -q, --quiet         Suppress the pipeline's output\n"
      "  -d, --decimals N    Set the number of decimal places to report "
      "statistics to (0-9)\n"
      "\n"
      "Statistics:\n"
      "  Return code\n"
      "  Wall-clock execution time\n"
      "  User CPU time\n"
      "  System CPU time\n"
      "  Peak memory usage\n"
      "\n"
      "Examples:\n"
      "  pipro \"ls | grep Make\"\n"
      "  pipro -s \"cat file.txt | sort | uniq\"\n"
      "  pipro -d 5 \"seq 1 1000000 | sort -n | tail\"\n"
      "\n"
      "Notes:\n"
      "  The entire pipeline must be passed as a single quoted argument.\n"
      "  Commands execute concurrently, so their wall-clock times will "
      "overlap.\n"
      "  Shell features such as redirection and complex quoting may not be "
      "supported.\n"
      "  Not all statistics can be given to all levels of precision\n");
}
