#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int run_pipeline(char ***pipeline, int command_count, pid_t *pids,
                 struct timespec *starts, int output_fd, int errors_fd) {
  // Creates pipes to be used between each command (n-1 pipes, 2 ends per pipe
  // as [read0, write0, read1, write1,...])
  int *pipefds = NULL;
  if (command_count != 1) {
    pipefds = malloc(2 * (command_count - 1) * sizeof(int));
    if (pipefds == NULL) {
      perror("malloc");
      return EXIT_FAILURE;
    }
  }

  for (int i = 0; i < command_count - 1; i++) {
    if (pipe(&pipefds[i * 2]) == -1) {
      perror("pipe");
      return EXIT_FAILURE;
    }
  }

  // Child processes started concurrently, one per command
  for (int i = 0; i < command_count; i++) {
    pid_t pid = fork();
    if (pid == -1) {
      perror("fork");
      return EXIT_FAILURE;
    }

    // Child process
    if (pid == 0) {
      // Redirect read end of previous pipe to stdin if not first command
      if (i != 0) {
        if (dup2(pipefds[(i - 1) * 2], STDIN_FILENO) == -1) {
          perror("dup2");
          return EXIT_FAILURE;
        }
      }

      // Redirect stdout to next pipe if not last command, else redirect output
      // to a file to print output
      if (i != command_count - 1) {
        if (dup2(pipefds[i * 2 + 1], STDOUT_FILENO) == -1) {
          perror("dup2");
          return EXIT_FAILURE;
        }
        close(output_fd);
      } else {
        if (dup2(output_fd, STDOUT_FILENO) == -1) {
          perror("dup2");
          exit(EXIT_FAILURE);
        }
      }

      // Redirect stderr to error file (error prints will interrupt output if
      // sent direct to terminal)
      if (dup2(errors_fd, STDERR_FILENO) == -1) {
        perror("dup2");
        exit(EXIT_FAILURE);
      }

      // Close all pipe ends, needed ones are already duplicated
      for (int j = 0; j < 2 * (command_count - 1); j++) {
        close(pipefds[j]);
      }

      execvp(pipeline[i][0], pipeline[i]);
      perror("execvp");
      _exit(127);
    }

    // Parent process (all that is left after child executes) saves pid to wait
    // later
    pids[i] = pid;

    // Record start time
    int time_return = clock_gettime(CLOCK_MONOTONIC, &starts[i]);
    if (time_return == EXIT_FAILURE) {
      perror("clock_gettime");
      return EXIT_FAILURE;
    }
  }

  // Close all remaining pipe ends
  for (int i = 0; i < 2 * (command_count - 1); i++) {
    close(pipefds[i]);
  }

  free(pipefds);

  return EXIT_SUCCESS;
}