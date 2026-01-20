// day08/term_vs_kill.c
#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t got_term = 0;

static void on_sigterm(int signo) {
    (void)signo;
    got_term = 1;
}

int main(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigterm;
    // 這裡開 SA_RESTART，避免輸出被打斷太多
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction(SIGTERM)");
        return 1;
    }

    printf("PID=%d\n", getpid());
    printf("Try in another terminal:\n");
    printf("  kill -TERM %d    (or: kill %d)\n", getpid(), getpid());
    printf("  kill -KILL %d    (or: kill -9 %d)\n", getpid(), getpid());
    printf("This program will IGNORE SIGTERM once, then exit gracefully.\n");

    while (1) {
        if (got_term) {
            got_term = 0;
            printf("[main] got SIGTERM: doing cleanup then exit(0)\n");
            // 模擬 cleanup
            sleep(1);
            printf("[main] cleanup done, bye\n");
            return 0;
        }

        printf("running...\n");
        sleep(1);
    }
}
