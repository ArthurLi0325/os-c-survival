#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        setpgid(0, 0);  // child 自己成為新的 group
        printf("[child] PID=%d, PGID=%d\n",
               getpid(), getpgid(0));
        while (1) sleep(1);
    } else {
        printf("[parent] PID=%d, PGID=%d\n",
               getpid(), getpgid(0));
        while (1) sleep(1);
    }
}