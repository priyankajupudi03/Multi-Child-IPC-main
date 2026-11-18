
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define SIZE 256

int main() {
    int p1[2], p2[2];
    pid_t pid;

    char message[SIZE];
    char response[SIZE];

    // Create two pipes
    if (pipe(p1) == -1 || pipe(p2) == -1) {
        perror("Pipe creation failed");
        return 1;
    }

    pid = fork();

    if (pid == -1) {
        perror("Fork failed");
        return 1;
    }

    // ================= CHILD PROCESS =================
    if (pid == 0) {
        close(p1[1]);
        close(p2[0]);

        printf("\n[CHILD] PID: %d\n", getpid());

        while (1) {
            memset(message, 0, SIZE);

            if (read(p1[0], message, SIZE) <= 0)
                break;

            message[SIZE - 1] = '\0';

            // Exit
            if (strcmp(message, "EXIT") == 0) {
                strcpy(response, "Child shutting down.");
                write(p2[1], response, SIZE);
                break;
            }

            // Child initiates chat
            if (strcmp(message, "CHILD_CHAT") == 0) {

                strcpy(response,
                       "Hello Parent! I want to talk.");

                write(p2[1], response, SIZE);

                // Wait for parent's reply
                memset(message, 0, SIZE);

                if (read(p1[0], message, SIZE) <= 0)
                    break;

                message[SIZE - 1] = '\0';

                snprintf(response, SIZE,
                         "Child received your reply: %.220s",
                         message);

                write(p2[1], response, SIZE);

                continue;
            }

            int a, b;

            // Addition
            if (sscanf(message, "ADD %d %d", &a, &b) == 2) {
                snprintf(response, SIZE,
                         "Addition: %d + %d = %d",
                         a, b, a + b);
            }

            // Subtraction
            else if (sscanf(message, "SUB %d %d", &a, &b) == 2) {
                snprintf(response, SIZE,
                         "Subtraction: %d - %d = %d",
                         a, b, a - b);
            }

            // Multiplication
            else if (sscanf(message, "MUL %d %d", &a, &b) == 2) {
                snprintf(response, SIZE,
                         "Multiplication: %d * %d = %d",
                         a, b, a * b);
            }

            // Division
            else if (sscanf(message, "DIV %d %d", &a, &b) == 2) {
                if (b == 0) {
                    strcpy(response,
                           "Error: Division by zero!");
                }
                else {
                    snprintf(response, SIZE,
                             "Division: %d / %d = %.2f",
                             a, b, (double)a / b);
                }
            }

            // Reverse text
            else if (strncmp(message, "REV ", 4) == 0) {
                char *text = message + 4;
                int len = strlen(text);

                strcpy(response, "Reverse: ");
                int pos = strlen(response);

                for (int i = len - 1; i >= 0; i--) {
                    response[pos++] = text[i];
                }

                response[pos] = '\0';
            }

            // Chat with child
            else if (strncmp(message, "CHAT ", 5) == 0) {
                char *text = message + 5;

                if (strcmp(text, "hello") == 0)
                    strcpy(response, "Hello Parent!");

                else if (strcmp(text, "how are you") == 0)
                    strcpy(response, "I am fine, Parent!");

                else if (strcmp(text, "who are you") == 0)
                    strcpy(response,
                           "I am a child process created using fork()!");

                else if (strcmp(text, "what is your name") == 0)
                    strcpy(response, "I am the Child Process.");

                else if (strcmp(text, "bye") == 0)
                    strcpy(response, "Goodbye, Parent!");

                else
                    strcpy(response,
                           "Child received your message successfully.");
            }

            // Child receives reply during child-initiated chat
            else if (strncmp(message, "PARENT_REPLY ", 13) == 0) {
                snprintf(response, SIZE,
                         "Parent replied: %.220s",
                         message + 13);
            }

            else {
                strcpy(response, "Unknown command.");
            }

            write(p2[1], response, SIZE);
        }

        close(p1[0]);
        close(p2[1]);

        exit(0);
    }

    // ================= PARENT PROCESS =================
    else {
        close(p1[0]);
        close(p2[1]);

        printf("[PARENT] PID: %d\n", getpid());
        printf("[PARENT] Child PID: %d\n", pid);

        while (1) {
            int choice, a, b;

            printf("\n===== ADVANCED IPC SYSTEM =====\n");
            printf("1. Addition\n");
            printf("2. Subtraction\n");
            printf("3. Multiplication\n");
            printf("4. Division\n");
            printf("5. Reverse text\n");
            printf("6. Chat with child\n");
            printf("7. Exit\n");
            printf("8. Child chat with parent\n");
            printf("Enter choice: ");

            if (scanf("%d", &choice) != 1)
                break;

            getchar();

            // Arithmetic
            if (choice >= 1 && choice <= 4) {
                printf("Enter two numbers: ");

                if (scanf("%d %d", &a, &b) != 2)
                    break;

                getchar();

                if (choice == 1)
                    snprintf(message, SIZE, "ADD %d %d", a, b);

                else if (choice == 2)
                    snprintf(message, SIZE, "SUB %d %d", a, b);

                else if (choice == 3)
                    snprintf(message, SIZE, "MUL %d %d", a, b);

                else
                    snprintf(message, SIZE, "DIV %d %d", a, b);
            }

            // Reverse text
            else if (choice == 5) {
                printf("Enter text: ");
                fgets(message, SIZE, stdin);

                message[strcspn(message, "\n")] = '\0';

                char temp[SIZE];
                snprintf(temp, SIZE, "REV %.240s", message);
                strcpy(message, temp);
            }

            // Chat with child
            else if (choice == 6) {
                printf("Enter message: ");
                fgets(message, SIZE, stdin);

                message[strcspn(message, "\n")] = '\0';

                char temp[SIZE];
                snprintf(temp, SIZE, "CHAT %.240s", message);
                strcpy(message, temp);
            }

            // Exit
            else if (choice == 7) {
                strcpy(message, "EXIT");
            }

            // Child initiates chat
            else if (choice == 8) {
                strcpy(message, "CHILD_CHAT");
            }

            else {
                printf("Invalid choice!\n");
                continue;
            }

            // Send message to child
            if (write(p1[1], message, SIZE) == -1) {
                perror("Pipe write failed");
                break;
            }

            // Receive child response
            memset(response, 0, SIZE);

            if (read(p2[0], response, SIZE) <= 0)
                break;

            response[SIZE - 1] = '\0';

            printf("\n[CHILD]: %s\n", response);

            // Child-initiated chat interaction
            if (choice == 8) {
                printf("[PARENT]: ");

                if (fgets(message, SIZE, stdin) == NULL)
                    break;

                message[strcspn(message, "\n")] = '\0';

                char temp[SIZE];
                snprintf(temp, SIZE,
                         "PARENT_REPLY %.240s", message);

                strcpy(message, temp);

                write(p1[1], message, SIZE);

                memset(response, 0, SIZE);

                if (read(p2[0], response, SIZE) <= 0)
                    break;

                response[SIZE - 1] = '\0';

                printf("[CHILD]: %s\n", response);
            }

            if (choice == 7)
                break;
        }

        close(p1[1]);
        close(p2[0]);

        wait(NULL);

        printf("\nIPC communication completed.\n");
    }

    return 0;
}
