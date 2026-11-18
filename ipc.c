
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

// Send complete message through pipe
int send_message(int fd, const char *msg)
{
    size_t len = strlen(msg) + 1;
    size_t sent = 0;

    while (sent < len)
    {
        ssize_t n = write(fd, msg + sent, len - sent);

        if (n == -1)
        {
            if (errno == EINTR)
                continue;

            perror("write failed");
            return -1;
        }

        if (n == 0)
        {
            fprintf(stderr, "write returned zero\n");
            return -1;
        }

        sent += n;
    }

    return 0;
}

// Receive message until null terminator
int receive_message(int fd, char *buffer, size_t size)
{
    size_t i = 0;

    while (i < size)
    {
        char ch;
        ssize_t n = read(fd, &ch, 1);

        if (n == -1)
        {
            if (errno == EINTR)
                continue;

            perror("read failed");
            return -1;
        }

        if (n == 0)
        {
            fprintf(stderr, "Pipe closed unexpectedly\n");
            return -1;
        }

        buffer[i++] = ch;

        if (ch == '\0')
            return 0;
    }

    fprintf(stderr, "Message too large\n");
    return -1;
}

int main()
{
    int p1[2], p2[2];
    pid_t pid;

    char message[100], reply[100];

    // Prevent termination if writing to a closed pipe
    signal(SIGPIPE, SIG_IGN);

    // Create first pipe
    if (pipe(p1) == -1)
    {
        perror("pipe p1 failed");
        return EXIT_FAILURE;
    }

    // Create second pipe
    if (pipe(p2) == -1)
    {
        perror("pipe p2 failed");
        close(p1[0]);
        close(p1[1]);
        return EXIT_FAILURE;
    }

    // Create child process
    pid = fork();

    if (pid == -1)
    {
        perror("fork failed");

        close(p1[0]);
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);

        return EXIT_FAILURE;
    }

    if (pid > 0)
    {
        // Parent process
        close(p1[0]);
        close(p2[1]);

        while (1)
        {
            printf("Parent: ");
            fflush(stdout);

            if (fgets(message, sizeof(message), stdin) == NULL)
            {
                strcpy(message, "exit");
                send_message(p1[1], message);
                break;
            }

            if (send_message(p1[1], message) == -1)
                break;

            if (receive_message(p2[0], reply, sizeof(reply)) == -1)
                break;

            printf("Child: %s", reply);

            if (strncmp(message, "exit", 4) == 0)
                break;
        }

        close(p1[1]);
        close(p2[0]);

        // Wait for child process
        if (waitpid(pid, NULL, 0) == -1)
            perror("waitpid failed");
    }
    else
    {
        // Child process
        close(p1[1]);
        close(p2[0]);

        while (1)
        {
            if (receive_message(p1[0], message, sizeof(message)) == -1)
                break;

            printf("Message received: %s", message);

            if (strncmp(message, "exit", 4) == 0)
            {
                strcpy(reply, "Goodbye!\n");

                if (send_message(p2[1], reply) == -1)
                    break;

                break;
            }

            printf("Child reply: ");
            fflush(stdout);

            if (fgets(reply, sizeof(reply), stdin) == NULL)
            {
                strcpy(reply, "Goodbye!\n");
                send_message(p2[1], reply);
                break;
            }

            if (send_message(p2[1], reply) == -1)
                break;
        }

        close(p1[0]);
        close(p2[1]);
    }

    return EXIT_SUCCESS;
}
