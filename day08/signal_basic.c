// day08/signal_basic.c
#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t got_sigint = 0;

static void on_sigint(int signo) {
    (void)signo;
    got_sigint = 1;
    // 注意：在 signal handler 裡用 printf 不是最佳實務（之後 Day08 Part2 會解釋）
    // 這裡先用最直觀方式讓你看到效果
    //const char *msg = "\n[handler] caught SIGINT (Ctrl+C)\n";
    //write(STDOUT_FILENO, msg, strlen(msg));
}

int main(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;

    // 讓被 signal 中斷的系統呼叫（例如 sleep）自動重啟（常用）
    //sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction(SIGINT)");
        return 1;
    }

    printf("PID = %d\n", getpid());
    printf("Try pressing Ctrl+C. Program should NOT exit.\n");
    printf("Press Ctrl+\\ (SIGQUIT) to quit, or run: kill -9 %d\n", getpid());

    while (1) {
        if (got_sigint){
            got_sigint = 0;
            printf("[main] observed SIGINT flag\n");
        }
        printf("working... (sleep 2)\n");
        unsigned int left = sleep(2);
        if (left != 0){
            printf("sleep interrupted! left = %u (errno=%d: %s)\n", 
                left, errno, strerror(errno));
        }
    }

    return 0;
}