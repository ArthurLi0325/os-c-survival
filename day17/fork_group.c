#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        printf("[child] PID=%d, PPID=%d, PGID=%d, SID=%d\n",
               getpid(), getppid(), getpgid(0), getsid(0));
        while (1) sleep(1);
    } else {
        printf("[parent] PID=%d, PPID=%d, PGID=%d, SID=%d\n",
               getpid(), getppid(), getpgid(0), getsid(0));
        while (1) sleep(1);
    }
}