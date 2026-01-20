// day08/sigchld.c
#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t got_sigchld = 0;

static void on_sigchld(int signo) {
    (void)signo;
    got_sigchld = 1;
}

int main(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigchld;

    // 先不開 SA_RESTART，讓你更容易觀察「被打斷」的行為
    // sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction(SIGCHLD)");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // child
        printf("[child] pid=%d: sleep 2 then exit(42)\n", getpid());
        sleep(2);
        printf("[child] exiting now\n");
        _exit(42);
    }

    // parent
    printf("[parent] pid=%d, child pid=%d\n", getpid(), pid);

    while (1) {
        if (got_sigchld) {
            got_sigchld = 0;

            // 用 WNOHANG：不要卡住，回收所有已結束的 child
            while (1) {
                int status;
                pid_t w = waitpid(-1, &status, WNOHANG);

                if (w > 0) {
                    if (WIFEXITED(status)) {
                        printf("[parent] reaped child %d, exit=%d\n",
                               w, WEXITSTATUS(status));
                    } else if (WIFSIGNALED(status)) {
                        printf("[parent] reaped child %d, killed by signal=%d\n",
                               w, WTERMSIG(status));
                    } else {
                        printf("[parent] reaped child %d, status=0x%x\n", w, status);
                    }
                    continue;
                }

                if (w == 0) {
                    // 沒有「已經結束」的 child 了
                    break;
                }

                // w < 0
                if (errno == EINTR) continue;
                if (errno == ECHILD) {
                    printf("[parent] no more children\n");
                    return 0; // demo 結束
                }

                printf("[parent] waitpid error: errno=%d (%s)\n", errno, strerror(errno));
                return 1;
            }
        }

        // 模擬 parent 還在做事
        printf("[parent] working...\n");
        sleep(1);
    }
}
