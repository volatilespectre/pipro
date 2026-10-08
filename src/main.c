#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE

#include "parser.h"
#include "pipeline.h"
#include "time_utils.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  // Set up variables to get from parser
  char ***pipeline;
  int command_count;
  int parser = parse_args(argc, argv, &pipeline, &command_count);

  // Parser will return exit failure if allocation fails or incorrect arguments
  // are entered
  if (parser == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }

  // Pipeline will return PIDs to wait for and start times for processes, as
  // well as sending the output of the last program to a temporary file for
  // printing
  FILE *output = tmpfile();
  if (output == NULL) {
    perror("tmpfile");
    return EXIT_FAILURE;
  }
  int output_fd = fileno(output);
  pid_t *pids = malloc(command_count * sizeof(pid_t));
  if (pids == NULL) {
    perror("malloc");
    return EXIT_FAILURE;
  }

  struct timespec *starts = malloc(command_count * sizeof(struct timespec));
  struct timespec *ends = malloc(command_count * sizeof(struct timespec));
  if (starts == NULL || ends == NULL) {
    perror("malloc");
    return EXIT_FAILURE;
  }

  int pipeline_result =
      run_pipeline(pipeline, command_count, pids, starts, output_fd);

  // Pipeline will return exit failure if system calls (pipe, fork, dup2) or
  // allocation fail
  if (pipeline_result == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }

  int *statuses = malloc(command_count * sizeof(int));
  if (statuses == NULL) {
    perror("malloc");
    return EXIT_FAILURE;
  }

  struct rusage usage;
  printf("%-20s %-20s %-20s %-20s %-22s", "Command name", "Exit Status",
         "User CPU (s)", "System CPU (s)", "Peak memory usage (MB)\n");
  for (int i = 0; i < command_count; i++) {
    // Wait for the first command to finish, and get resource usage stats
    if (wait4(pids[i], &statuses[i], 0, &usage) == -1) {
      perror("wait4");
      return EXIT_FAILURE;
    }

    // Calculate the time it took to run
    int clock_return = clock_gettime(CLOCK_MONOTONIC, &ends[i]);
    if (clock_return == EXIT_FAILURE) {
      perror("clock_gettime");
      return EXIT_FAILURE;
    }

    double runtime;
    timespec_subtract(&runtime, &starts[i], &ends[i], 3);

    double user_cpu =
        usage.ru_utime.tv_sec + (usage.ru_utime.tv_usec / 1000000.0);
    double sys_cpu =
        usage.ru_stime.tv_sec + (usage.ru_stime.tv_usec / 1000000.0);

    long peak_mem_units = usage.ru_maxrss;
    double peak_mem_mb;

#if defined(__APPLE__) && defined(__MACH__)
    // MacOS gives ru_maxrss in bytes
    peak_mem_mb = peak_mem_units / (1024.0 * 1024.0);
#else
    // Linux and BSDs give ru_maxrss in kilobytes
    peak_mem_mb = peak_mem_units / 1024.0;
#endif

    char status_str[16];
    if (WIFEXITED(statuses[i])) {
      snprintf(status_str, sizeof(status_str), "Returned %d",
               WEXITSTATUS(statuses[i]));
    } else if (WIFSIGNALED(statuses[i])) {
      snprintf(status_str, sizeof(status_str), "Signal (%d)",
               WTERMSIG(statuses[i]));
    } else {
      snprintf(status_str, sizeof(status_str), "UNKNOWN");
    }

    printf("%-20s %-20s %-20.3f %-20.3f %-20.3f\n", pipeline[i][0], status_str,
           user_cpu, sys_cpu, peak_mem_mb);
  }

  if (fseek(output, 0, SEEK_SET) != 0) {
    perror("fseek");
    return EXIT_FAILURE;
  }
  printf("---------- OUTPUT ----------\n");
  int c;
  while ((c = fgetc(output)) != EOF) {
    putchar(c);
  }
  fclose(output);

  // Free all memory allocations
  free(pids);
  free(starts);
  free(ends);
  free(statuses);

  return EXIT_SUCCESS;
}
