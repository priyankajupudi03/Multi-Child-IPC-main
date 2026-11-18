

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define SIZE 100

// ANSI colors
#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define BLUE    "\033[1;34m"
#define YELLOW  "\033[1;33m"
#define CYAN    "\033[1;36m"
#define RESET   "\033[0m"

void delay(int seconds)
{
    sleep(seconds);
}

void show_arrow(const char *message, int direction)
{
    printf("\n");

    if (direction == 1)
    {
        printf(BLUE "PARENT" RESET "  ");
        printf("==== Pipe 1: Parent -> Child ====> ");
        printf(GREEN "CHILD\n" RESET);
    }
    else
    {
        printf(GREEN "CHILD" RESET "   ");
        printf("==== Pipe 2: Child -> Parent ====> ");
        printf(BLUE "PARENT\n" RESET);
    }

    printf(YELLOW "Message: %s\n" RESET, message);
    fflush(stdout);
    delay(2);
}

int main()
{
    int pipe1[2]; // Parent -> Child
    int pipe2[2]; // Child -> Parent

    char message[SIZE];
    char response[SIZE];

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1)
    {
        perror("Pipe creation failed");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        perror("Fork failed");
        return 1;
    }

    if (pid == 0)
    {
        // CHILD PROCESS

        close(pipe1[1]);
        close(pipe2[0]);

        printf("\n" GREEN "========== CHILD PROCESS ==========\n" RESET);
        printf("Child PID: %d\n", getpid());
        delay(1);

        read(pipe1[0], message, sizeof(message));

        printf(CYAN "\nChild received message: %s\n" RESET, message);
        delay(2);

        strcpy(response, "Hello Parent! Message received successfully.");

        show_arrow(response, 0);

        write(pipe2[1], response, strlen(response) + 1);

        close(pipe1[0]);
        close(pipe2[1]);

        exit(0);
    }
    else
    {
        // PARENT PROCESS

        close(pipe1[0]);
        close(pipe2[1]);

        printf("\n" BLUE "========== PARENT PROCESS ==========\n" RESET);
        printf("Parent PID: %d\n", getpid());
        printf("Child PID : %d\n", pid);

        strcpy(message, "Hello Child! This is Parent.");

        show_arrow(message, 1);

        write(pipe1[1], message, strlen(message) + 1);

        read(pipe2[0], response, sizeof(response));

        printf("\n" BLUE "Parent received response: %s\n" RESET, response);

        wait(NULL);

        printf("\n" GREEN "========== IPC COMPLETED ==========\n" RESET);

        close(pipe1[1]);
        close(pipe2[0]);
    }

    return 0;
}
