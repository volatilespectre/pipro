#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("You haven't entered a command to be debugged. ");
        return EXIT_SUCCESS;
    }

    int count = 0;
    int capacity = 5;
    char **tokens = malloc(capacity * sizeof(char *));
    if (tokens == NULL) {
	    perror("malloc");
	    return EXIT_FAILURE;
    }
    char *current = argv[1];

    while (*current != '\0') {
        while (isspace(*current)) {
            current++;
        }

        char *start = current;
        while(*current != '\0' && !isspace(*current)) {
            current++;
        }

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

    int command_count = 1
    ;
    for (int i = 0; i < count; i++) {
        if (strcmp(tokens[i], "|") == 0) {
            command_count++;
        }
    }
    char ***pipeline = malloc(command_count * sizeof(char **));
    if (pipeline == NULL) {
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
        pipeline[command_number] = malloc((len + 1) * sizeof(char *));
        if (pipeline[command_number] == NULL) {
            perror("malloc");
            return EXIT_FAILURE;
        }
        for (int j = 0; j < len; j++) {
            pipeline[command_number][j] = tokens[start + j];
        }
        pipeline[command_number][len] = NULL;

        start = i + 1;
        command_number++;
    }
    for (int i = 0; i < command_count; i++) {
        pid_t pid = fork();
	if (pid == 0) {
            execvp(pipeline[i][0], pipeline[i]);
	}
    }
}
