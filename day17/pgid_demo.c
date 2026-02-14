#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    printf("PID=%d, PPID=%d, PGID=%d\n",
           getpid(),
           getppid(),
           getpgid(0));

    sleep(30);
    return 0;
}
