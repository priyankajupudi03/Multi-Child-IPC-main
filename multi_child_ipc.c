
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define CHILDREN 3
#define TEXT_SIZE 256

// Message structure
typedef struct {
    int operation;
    double a;
    double b;
    double result;
    char text[TEXT_SIZE];
} Message;

// Write complete data to pipe
int write_all(int fd, const void *buffer, size_t size) {
    const char *ptr = buffer;
    size_t total = 0;

    while (total < size) {
        ssize_t n = write(fd, ptr + total, size - total);

        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        if (n == 0)
            return -1;

        total += n;
    }

    return 0;
}

// Read complete data from pipe
int read_all(int fd, void *buffer, size_t size) {
    char *ptr = buffer;
    size_t total = 0;

    while (total < size) {
        ssize_t n = read(fd, ptr + total, size - total);

        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        if (n == 0)
            return -1;

        total += n;
    }

    return 0;
}

// Close unused pipes in each child
void close_child_pipes(int p2c[CHILDREN][2],
                       int c2p[CHILDREN][2],
                       int index) {

    for (int i = 0; i < CHILDREN; i++) {

        if (i != index) {
            close(p2c[i][0]);
            close(p2c[i][1]);
            close(c2p[i][0]);
            close(c2p[i][1]);
        } else {
            close(p2c[i][1]);
            close(c2p[i][0]);
        }
    }
}

// Child process
void child_process(int index,
                   int p2c[CHILDREN][2],
                   int c2p[CHILDREN][2]) {

    close_child_pipes(p2c, c2p, index);

    int input_fd = p2c[index][0];
    int output_fd = c2p[index][1];

    Message request, response;

    printf("\n[Child %d] Started. Waiting for requests...\n",
           index + 1);
    fflush(stdout);

    while (1) {

        if (read_all(input_fd, &request, sizeof(request)) == -1)
            break;

        memset(&response, 0, sizeof(response));

        // Termination command
        if (request.operation == 0) {
            strcpy(response.text, "Child terminated.");
            write_all(output_fd, &response, sizeof(response));
            break;
        }

        // Child 1: Addition and Subtraction
        if (index == 0) {

            if (request.operation == 1) {
                response.result = request.a + request.b;
                strcpy(response.text, "Addition completed.");
            }
            else if (request.operation == 2) {
                response.result = request.a - request.b;
                strcpy(response.text, "Subtraction completed.");
            }
        }

        // Child 2: Multiplication and Division
        else if (index == 1) {

            if (request.operation == 3) {
                response.result = request.a * request.b;
                strcpy(response.text, "Multiplication completed.");
            }
            else if (request.operation == 4) {

                if (request.b == 0) {
                    strcpy(response.text,
                           "Error: Division by zero is not allowed.");
                }
                else {
                    response.result = request.a / request.b;
                    strcpy(response.text, "Division completed.");
                }
            }
        }

        // Child 3: Chat communication
        else if (index == 2) {

            if (request.operation == 5) {

                printf("\n[Child 3] Message from Parent: %s\n",
                       request.text);

                printf("[Child 3] Enter your reply: ");
                fflush(stdout);

                if (fgets(response.text, TEXT_SIZE, stdin) == NULL)
                    strcpy(response.text, "No reply entered.");

                response.text[strcspn(response.text, "\n")] = '\0';
            }
        }

        // Send response to parent
        if (write_all(output_fd, &response, sizeof(response)) == -1)
            break;
    }

    close(input_fd);
    close(output_fd);

    printf("[Child %d] Exiting.\n", index + 1);
    fflush(stdout);

    exit(0);
}

// Get numeric input
double get_number(const char *message) {
    char line[100];
    char *end;
    double value;

    while (1) {
        printf("%s", message);

        if (fgets(line, sizeof(line), stdin) == NULL)
            exit(1);

        value = strtod(line, &end);

        while (*end == ' ' || *end == '\t' || *end == '\n')
            end++;

        if (end != line && *end == '\0')
            return value;

        printf("Invalid number. Try again.\n");
    }
}

// Get menu choice
int get_choice(void) {
    char line[100];
    char *end;
    long choice;

    while (1) {
        printf("\nEnter your choice: ");

        if (fgets(line, sizeof(line), stdin) == NULL)
            return 6;

        choice = strtol(line, &end, 10);

        while (*end == ' ' || *end == '\t' || *end == '\n')
            end++;

        if (end != line && *end == '\0' &&
            choice >= 1 && choice <= 6)
            return (int)choice;

        printf("Invalid choice. Enter 1 to 6.\n");
    }
}

// Main function
int main(void) {

    int p2c[CHILDREN][2];
    int c2p[CHILDREN][2];
    pid_t pid;

    Message request, response;

    printf("====================================\n");
    printf("   MULTI-CHILD BIDIRECTIONAL IPC\n");
    printf("====================================\n");

    // Create six pipes
    for (int i = 0; i < CHILDREN; i++) {

        if (pipe(p2c[i]) == -1) {
            perror("Pipe creation failed");
            exit(1);
        }

        if (pipe(c2p[i]) == -1) {
            perror("Pipe creation failed");
            exit(1);
        }
    }

    printf("\n[Parent] Six pipes created successfully.\n");

    // Create three child processes
    for (int i = 0; i < CHILDREN; i++) {

        pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        }

        if (pid == 0) {
            child_process(i, p2c, c2p);
        }
    }

    // Parent closes unused pipe ends
    for (int i = 0; i < CHILDREN; i++) {
        close(p2c[i][0]);
        close(c2p[i][1]);
    }

    printf("\n[Parent] Three child processes created successfully.\n");

    // Main menu
    while (1) {

        printf("\n========== MAIN MENU ==========\n");
        printf("1. Addition        (Child 1)\n");
        printf("2. Subtraction     (Child 1)\n");
        printf("3. Multiplication  (Child 2)\n");
        printf("4. Division        (Child 2)\n");
        printf("5. Chat Mode       (Child 3)\n");
        printf("6. Exit\n");
        printf("===============================\n");

        int choice = get_choice();

        // Exit: terminate all three children
        if (choice == 6) {

            printf("\n[Parent] Sending termination signals...\n");

            for (int i = 0; i < CHILDREN; i++) {

                memset(&request, 0, sizeof(request));
                memset(&response, 0, sizeof(response));

                request.operation = 0;

                if (write_all(p2c[i][1],
                              &request, sizeof(request)) == -1) {
                    perror("Termination request failed");
                    continue;
                }

                if (read_all(c2p[i][0],
                             &response, sizeof(response)) == -1) {
                    perror("Termination response failed");
                }

                printf("[Parent] Child %d termination signal sent.\n",
                       i + 1);
            }

            break;
        }

        memset(&request, 0, sizeof(request));
        memset(&response, 0, sizeof(response));

        int child_index;

        if (choice == 1 || choice == 2)
            child_index = 0;

        else if (choice == 3 || choice == 4)
            child_index = 1;

        else
            child_index = 2;

        request.operation = choice;

        // Arithmetic input
        if (choice >= 1 && choice <= 4) {

            request.a = get_number("Enter first number: ");
            request.b = get_number("Enter second number: ");
        }

        // Chat input
        else if (choice == 5) {

            printf("Enter message to Child 3: ");

            if (fgets(request.text, TEXT_SIZE, stdin) == NULL)
                strcpy(request.text, "");

            request.text[strcspn(request.text, "\n")] = '\0';
        }

        // Send request to selected child
        printf("\n[Parent] Sending request to Child %d...\n",
               child_index + 1);

        if (write_all(p2c[child_index][1],
                      &request, sizeof(request)) == -1) {
            perror("Parent write failed");
            break;
        }

        // Receive response from selected child
        if (read_all(c2p[child_index][0],
                     &response, sizeof(response)) == -1) {
            perror("Parent read failed");
            break;
        }

        printf("\n[Parent] Response received from Child %d.\n",
               child_index + 1);

        if (choice >= 1 && choice <= 4) {
            printf("Result: %.2f\n", response.result);
            printf("Status: %s\n", response.text);
        }

        else if (choice == 5) {
            printf("Child 3 replied: %s\n", response.text);
        }
    }

    // Close remaining parent pipe ends
    for (int i = 0; i < CHILDREN; i++) {
        close(p2c[i][1]);
        close(c2p[i][0]);
    }

    // Wait for all child processes
    for (int i = 0; i < CHILDREN; i++) {
        wait(NULL);
    }

    printf("\n[Parent] All child processes terminated.\n");
    printf("[Parent] IPC program completed successfully.\n");

    return 0;
}
