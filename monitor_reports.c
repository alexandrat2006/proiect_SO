#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void handle_sigusr1(int sig) {
    printf("[MONITOR] Notificare primita: Un nou raport a fost adaugat!\n");
    fflush(stdout);
}

void handle_sigint(int sig) {
    printf("\n[MONITOR] Se inchide... Sterg .monitor_pid\n");
    fflush(stdout);
    unlink(".monitor_pid");
    exit(0);
}

int main() {
    FILE *check_f = fopen(".monitor_pid", "r");
    if (check_f) {
        int existing_pid;
        if (fscanf(check_f, "%d", &existing_pid) == 1) {
            if (kill(existing_pid, 0) == 0) {
                printf("ERROR: ALREADY RUNNING\n");
                fflush(stdout);
                fclose(check_f);
                return 1;
            }
        }
        fclose(check_f);
    }

    FILE *f = fopen(".monitor_pid", "w");
    if (!f) {
        perror("Eroare deschidere fisier");
        return 1;
    }
    fprintf(f, "%d", getpid());
    fclose(f);

    struct sigaction sa_usr1, sa_int;
    
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    printf("Monitor activ (PID: %d). Astept semnale...\n", getpid());
    fflush(stdout);

    while(1) {
        pause();
    }
    return 0;
}
