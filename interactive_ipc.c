
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define SIZE 256

#define BLUE   "\033[1;34m"
#define GREEN  "\033[1;32m"
#define YELLOW "\033[1;33m"
#define CYAN   "\033[1;36m"
#define RED    "\033[1;31m"
#define RESET  "\033[0m"

typedef struct
{
    int operation;
    double num1;
    double num2;
    char text[SIZE];
} Message;

// Read complete message from pipe
int read_message(int fd, Message *msg)
{
    char *ptr = (char *)msg;
    size_t remaining = sizeof(Message);

    while (remaining > 0)
    {
        ssize_t n = read(fd, ptr, remaining);

        if (n <= 0)
            return -1;

        ptr += n;
        remaining -= n;
    }

    return 0;
}

// Write complete message to pipe
int write_message(int fd, Message *msg)
{
    char *ptr = (char *)msg;
    size_t remaining = sizeof(Message);

    while (remaining > 0)
    {
        ssize_t n = write(fd, ptr, remaining);

        if (n <= 0)
            return -1;

        ptr += n;
        remaining -= n;
    }

    return 0;
}

// Animated pipe visualization
void animate_pipe(int direction)
{
    if (direction == 1)
    {
        printf(BLUE "\nPARENT " RESET "==== Pipe 1 ");

        for (int i = 0; i < 3; i++)
        {
            printf("---->");
            fflush(stdout);
            usleep(200000);
        }

        printf(GREEN " CHILD\n" RESET);
    }
    else
    {
        printf(GREEN "\nCHILD " RESET "==== Pipe 2 ");

        for (int i = 0; i < 3; i++)
        {
            printf("---->");
            fflush(stdout);
            usleep(200000);
        }

        printf(BLUE " PARENT\n" RESET);
    }
}

// Child process
void child_process(int pipe1[], int pipe2[])
{
    Message request, response;

    close(pipe1[1]);
    close(pipe2[0]);

    printf(GREEN "\n========== CHILD PROCESS ==========\n" RESET);
    printf("Child PID: %d\n", getpid());
    fflush(stdout);

    while (1)
    {
        memset(&request, 0, sizeof(request));
        memset(&response, 0, sizeof(response));

        if (read_message(pipe1[0], &request) == -1)
            break;

        printf(CYAN "\nChild received request.\n" RESET);
        printf("Operation: %d\n", request.operation);

        switch (request.operation)
        {
            case 1:
                response.num1 = request.num1 + request.num2;
                snprintf(response.text, SIZE,
                         "Addition: %.2f + %.2f = %.2f",
                         request.num1, request.num2,
                         response.num1);
                break;

            case 2:
                response.num1 = request.num1 - request.num2;
                snprintf(response.text, SIZE,
                         "Subtraction: %.2f - %.2f = %.2f",
                         request.num1, request.num2,
                         response.num1);
                break;

            case 3:
                response.num1 = request.num1 * request.num2;
                snprintf(response.text, SIZE,
                         "Multiplication: %.2f * %.2f = %.2f",
                         request.num1, request.num2,
                         response.num1);
                break;

            case 4:
                if (request.num2 == 0)
                {
                    strcpy(response.text,
                           "Error: Division by zero!");
                }
                else
                {
                    response.num1 = request.num1 / request.num2;

                    snprintf(response.text, SIZE,
                             "Division: %.2f / %.2f = %.2f",
                             request.num1, request.num2,
                             response.num1);
                }
                break;

            case 5:
                printf(CYAN "Parent says: %s\n" RESET,
                       request.text);

                printf(YELLOW
                       "\nEnter Child's message: "
                       RESET);
                fflush(stdout);

                if (fgets(response.text, SIZE, stdin) == NULL)
                    strcpy(response.text, "Child message unavailable.");

                response.text[strcspn(response.text, "\n")] = '\0';
                break;

            case 6:
                strcpy(response.text,
                       "Child process shutting down.");
                break;

            default:
                strcpy(response.text, "Invalid operation.");
        }

        printf(YELLOW "Child processing...\n" RESET);
        sleep(1);

        animate_pipe(0);

        if (write_message(pipe2[1], &response) == -1)
            break;

        if (request.operation == 6)
            break;
    }

    close(pipe1[0]);
    close(pipe2[1]);

    printf(RED "\nChild process terminated.\n" RESET);
    exit(0);
}

int main()
{
    int pipe1[2]; // Parent -> Child
    int pipe2[2]; // Child -> Parent

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
        child_process(pipe1, pipe2);
    }
    else
    {
        Message request, response;
        char input[SIZE];

        close(pipe1[0]);
        close(pipe2[1]);

        printf(BLUE "\n========== PARENT PROCESS ==========\n" RESET);
        printf("Parent PID: %d\n", getpid());
        printf("Child PID : %d\n", pid);

        while (1)
        {
            printf(YELLOW
                   "\n========== IPC MENU ==========\n"
                   RESET);

            printf("1. Addition\n");
            printf("2. Subtraction\n");
            printf("3. Multiplication\n");
            printf("4. Division\n");
            printf("5. Chat Mode\n");
            printf("6. Exit\n");
            printf("Enter choice: ");

            fflush(stdout);

            if (fgets(input, SIZE, stdin) == NULL)
                break;

            if (sscanf(input, "%d", &request.operation) != 1)
            {
                printf(RED "Invalid input.\n" RESET);
                continue;
            }

            memset(&request, 0, sizeof(request));

            // Read choice again after clearing structure
            sscanf(input, "%d", &request.operation);

            if (request.operation >= 1 &&
                request.operation <= 4)
            {
                printf("Enter two numbers: ");

                if (fgets(input, SIZE, stdin) == NULL)
                    break;

                if (sscanf(input, "%lf %lf",
                           &request.num1,
                           &request.num2) != 2)
                {
                    printf(RED "Invalid numbers.\n" RESET);
                    continue;
                }
            }
            else if (request.operation == 5)
            {
                printf("Enter Parent's message: ");

                if (fgets(request.text, SIZE, stdin) == NULL)
                    break;

                request.text[strcspn(request.text, "\n")] = '\0';
            }
            else if (request.operation == 6)
            {
                strcpy(request.text, "EXIT");
            }
            else
            {
                printf(RED "Invalid menu option.\n" RESET);
                continue;
            }

            animate_pipe(1);

            if (write_message(pipe1[1], &request) == -1)
                break;

            memset(&response, 0, sizeof(response));

            if (read_message(pipe2[0], &response) == -1)
                break;

            printf(GREEN "\nParent received: %s\n" RESET,
                   response.text);

            if (request.operation == 6)
                break;
        }

        close(pipe1[1]);
        close(pipe2[0]);

        wait(NULL);

        printf(GREEN
               "\n========== IPC COMPLETED ==========\n"
               RESET);
    }

    return 0;
}
