#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE

#include "output.h"
#include "parser.h"
#include "pipeline.h"
#include "time_utils.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  // Set up variables to get from parser
  char ***pipeline;
  int command_count;
  int parser = parse_pipeline(argc, argv, &pipeline, &command_count);

  // Parser will return exit failure if allocation fails or incorrect arguments
  // are entered
  if (parser == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }

  // Pipeline will return PIDs to wait for and start times for processes, as
  // well as sending the output of the last program to a temporary file for
  // printing
  // Note that the pipeline contains allocations that must be freed here, as
  // their values are needed
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
  if (starts == NULL) {
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

  // Output will return exit failure if allocation fails
  int output_return = show_output(pipeline, command_count, pids, starts);
  if (output_return == EXIT_FAILURE) {
    return EXIT_FAILURE;
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

  for (int i = 0; i < command_count; i++) {
    int j = 0;
    while (pipeline[i][j] != NULL) {
      free(pipeline[i][j]);
      j++;
    }
    free(pipeline[i]);
  }

  return EXIT_SUCCESS;
}
