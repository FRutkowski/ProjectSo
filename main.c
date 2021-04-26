#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

char* readLine(int fd) {
    size_t size = 16;
    size_t len  = 0;

    char temp;
    char *buf = malloc(size * sizeof(char));

    do {
        if(len >= size) {
            size *= 2;
            buf = realloc(buf, size);

            if (buf == NULL)
                return NULL;
        }

        if (read(fd, &temp, 1) == 0) {
            printf("\n");
            exit(EXIT_SUCCESS);
        }

        buf[len++] = temp;
    } while (temp != '\n');

    buf[len - 1] = '\0';
    return buf;
}

char** splitString(char* str, char* delim, int* count) {
    int size = 2;
    *count = 0;

    char** args = malloc(size * sizeof(char*));
    char* ptr = strtok(str, delim);

    for (int i = 0; ptr != NULL; i++) {
        if((i + 1) >= size) {
            size *= 2;
            args = realloc(args, size * sizeof(char*));
        }

        args[i] = ptr;
        args[i + 1] = NULL;

        ptr = strtok(NULL, delim);
        (*count)++;
    }

    return args;
}

void loop() {
    int count = 0;

    while (1) {
        int flag = 0;

        char* input = readLine(STDIN_FILENO);
        char** strings = splitString(input, "|", &count);

        int* pipes = malloc((count - 1) * 2 * sizeof(int));

        for (int i = 0; i < (count - 1) * 2; i += 2) {
            pipe(pipes + i);
        }

        int programCount;
        char** program = splitString(strings[0], " ", &programCount);

        char* temp = malloc(1 * sizeof(char));
        *temp = strings[0][0];

        if (strcmp(temp, "#") == 0)
            continue;

        if (strcmp(program[programCount - 1], "&") == 0) {
            program[--programCount] = NULL;
            flag = 1;
        }

        if (fork() == 0) {
            if(count > 1)
                dup2(pipes[1], STDOUT_FILENO);

            if (programCount > 2 && strcmp(program[programCount - 2], ">>") == 0) {
                int fd = open(program[programCount - 1], O_CREAT | O_WRONLY | O_APPEND, 0666);
                dup2(fd, STDOUT_FILENO);

                program[programCount - 2] = NULL;
                program[programCount - 1] = NULL;
                programCount -= 2;
            }

            for (int i = 0; i < (count - 1) * 2; i += 2) {
                close(pipes[i]);
                close(pipes[i + 1]);
            }

            execvp(program[0], program);

            perror("execvp");
            exit(EXIT_FAILURE);

        }

        for (int i = 0; i < (count - 2); i++) {
            if (fork() == 0) {
                program = splitString(strings[i + 1], " ", &programCount);

                dup2(pipes[0 + (i * 2)], STDIN_FILENO);
                dup2(pipes[3 + (i * 2)], STDOUT_FILENO);

                for (int i = 0; i < (count - 1) * 2; i += 2) {
                    close(pipes[i]);
                    close(pipes[i + 1]);
                }

                execvp(program[0], program);

                perror("execvp");
                exit(EXIT_FAILURE);

            }
        }

        if (count > 1) {
            if(fork() == 0) {
                program = splitString(strings[count - 1], " ", &programCount);

                if (programCount > 2 && strcmp(program[programCount - 2], ">>") == 0) {
                    int fd = open(program[programCount - 1], O_CREAT | O_WRONLY | O_APPEND, 0666);
                    dup2(fd, STDOUT_FILENO);

                    program[programCount - 2] = NULL;
                    program[programCount - 1] = NULL;
                    programCount -= 2;
                }

                dup2(pipes[(count - 2) * 2], STDIN_FILENO);

                for (int i = 0; i < (count - 1) * 2; i += 2) {
                    close(pipes[i]);
                    close(pipes[i + 1]);
                }

                execvp(program[0], program);

                perror("execvp");
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < (count - 1) * 2; i += 2) {
            close(pipes[i]);
            close(pipes[i + 1]);
        }

        while (flag == 0 && waitpid(-1, NULL, 0) != -1);
    }
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        loop();
    } else if (argc >= 2) {
        int fd = open(argv[1], O_RDONLY, 0666);
        dup2(fd, STDIN_FILENO);
        loop();
    }

    return 0;
}