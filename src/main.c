#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "parser.h"

int main(int argc, char *argv[]) {
    // Set up variables to get from parser
    char ***pipeline;
    int command_count;
    int parser = parse_args(argc, argv, &pipeline, &command_count);

    // Parser will return exit failure if allocation fails or incorrect arguments are entered
    if (parser == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    // Creates pipes to be used between each command (n-1 pipes, 2 ends per pipe as [read0, write0, read1, write1,...])
    int *pipefds = malloc(2 * (command_count - 1) * sizeof(int));
    for (int i = 0; i < command_count - 1; i++) {
        if (pipe(&pipefds[i * 2]) == -1) {
            perror("pipe");
            return EXIT_FAILURE;
        }
    }

    pid_t *pids = malloc(command_count * sizeof(pid_t));
    for (int i = 0; i < command_count; i++) {
        // Creates child to execute this command
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return EXIT_FAILURE;
        }

        // Child process
        if (pid == 0) {
            // Redirect read end of previous pipe to stdin if not first command
            if (i != 0) {
                if (dup2(pipefds[(i - 1) * 2], STDIN_FILENO)) {
                    perror("dup2");
                    return EXIT_FAILURE;
                }
            }

            // Redirect stdout to next pipe if not last command
            if (i != command_count - 1) {
                if (dup2(pipefds[i * 2 + 1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    return EXIT_FAILURE;
                }
            }

            // Close all pipe ends, needed ones are already duplicated
            for (int i = 0; i < 2 * (command_count - 1); i++) {
                close(pipefds[i]);
            }

            execvp(pipeline[i][0], pipeline[i]);
            perror("execvp");
            return EXIT_FAILURE;
        }

        // Parent process (all that is left after child executes) saves pid to wait later
        pids[i] = pid;
    }

    // Close all remaining pipe ends
    for (int i = 0; i < 2 * (command_count - 1); i++) {
        close(pipefds[i]);
    }

    int *statuses = malloc(command_count * sizeof(int));
    for (int i = 0; i < command_count; i++) {
        waitpid(pids[i], &statuses[i], 0);
    }
    return 0;
}