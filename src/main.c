#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "parser.h"
#include "pipeline.h"

int main(int argc, char *argv[]) {
    // Set up variables to get from parser
    char ***pipeline;
    int command_count;
    int parser = parse_args(argc, argv, &pipeline, &command_count);

    // Parser will return exit failure if allocation fails or incorrect arguments are entered
    if (parser == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    // Pipeline will return PIDs to wait for
    pid_t *pids = malloc(command_count * sizeof(pid_t));
    if (pids == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }
    int pipeline_result = run_pipeline(pipeline, command_count, pids);
    
    // Pipeline will return exit failure if system calls (pipe, fork, dup2) or allocation fail
    if (pipeline_result == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    

    int *statuses = malloc(command_count * sizeof(int));
    if (statuses == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < command_count; i++) {
        waitpid(pids[i], &statuses[i], 0);
    }

    for (int i = 0; i < command_count; i++) {
        if (WIFEXITED(statuses[i])) {
            int code = WEXITSTATUS(statuses[i]);
            printf("%s exited with status %d\n", pipeline[i][0], code);
        } else if (WIFSIGNALED(statuses[i])) {
            int sig = WTERMSIG(statuses[i]);
            printf("%s killed by signal %d\n", pipeline[i][0], sig);
        }
    }

    return EXIT_SUCCESS;
}