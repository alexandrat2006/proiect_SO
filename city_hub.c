#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_DISTRICTS 10
#define BUF_SIZE 1024

void start_monitor() {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("Pipe failed");
        return;
    }
    pid_t hub_mon_pid = fork();
    if (hub_mon_pid == -1) {
        perror("First fork failed");
        return;
    }
    if (hub_mon_pid == 0) {
        pid_t monitor_pid = fork();
        if (monitor_pid == -1) {
            perror("Second fork failed");
            exit(1);
        }
        if (monitor_pid == 0) {
            // Monitorul scrie în pipe
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
            close(fd[0]);
            execl("./monitor_reports", "./monitor_reports", NULL);
            perror("execl monitor failed");
            exit(1);
        }
        close(fd[1]);
        
        char buffer[BUF_SIZE];
        ssize_t n;
        while ((n = read(fd[0], buffer, BUF_SIZE - 1)) > 0) {
            buffer[n] = '\0';
            if (strstr(buffer, "ERROR") || strstr(buffer, "ALREADY RUNNING")) {
                printf("\n[HUB_MON_ALERT] Monitor stopped: %s\nhub> ", buffer);
            } else {
                printf("\n[MONITOR_SYSTEM]: %s\nhub> ", buffer);
            }
            fflush(stdout);
        }
        close(fd[0]);
        waitpid(monitor_pid, NULL, 0);
        printf("\n[HUB_MON] Monitor process ended.\nhub> ");
        fflush(stdout);
        exit(0);
    }
    
    close(fd[0]);
    close(fd[1]);
    printf("[HUB] Monitor background process started successfully.\n");
}
void calculate_scores(int argc, char *argv[]) {
    printf("\n--- Starting Workload Analysis ---\n");

    for (int i = 1; i < argc; i++) {
        int fd[2];
        if (pipe(fd) == -1) continue;

        pid_t pid = fork();
        if (pid == 0) {
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);

            execl("./scorer", "./scorer", argv[i], NULL);
            fprintf(stderr, "Error executing scorer for %s\n", argv[i]);
            exit(1);
        }

        close(fd[1]);
        char buffer[BUF_SIZE];
        ssize_t bytes_read;

        printf("District: [%s]\n", argv[i]);
        while ((bytes_read = read(fd[0], buffer, BUF_SIZE - 1)) > 0) {
            buffer[bytes_read] = '\0';
            printf("%s", buffer);
        }

        close(fd[0]);
        waitpid(pid, NULL, 0);
        printf("----------------------------------\n");
    }
}

int main() {
    char input[256];
    char *args[MAX_DISTRICTS + 1];

    printf("City Infrastructure Hub Active\n");
    printf("Commands: start_monitor, calculate_scores <d1> <d2> ..., exit\n");

    while (1) {
        printf("\nhub> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;

        int count = 0;
        char *token = strtok(input, " ");
        while (token != NULL && count < MAX_DISTRICTS) {
            args[count++] = token;
            token = strtok(NULL, " ");
        }
        args[count] = NULL;

        if (count == 0) continue;

        if (strcmp(args[0], "exit") == 0) {
            break;
        } else if (strcmp(args[0], "start_monitor") == 0) {
            start_monitor();
        } else if (strcmp(args[0], "calculate_scores") == 0) {
            if (count < 2) {
                printf("Usage: calculate_scores <district_id1> [district_id2]...\n");
            } else {
                calculate_scores(count, args);
            }
        } else {
            printf("Unknown command: %s\n", args[0]);
        }
    }

    return 0;
}
