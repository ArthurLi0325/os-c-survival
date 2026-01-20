// day08/zombie_demo.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // child: 立刻結束
        printf("[child] pid=%d: exiting now\n", getpid());
        _exit(0);
    }

    // parent: 故意不 wait
    printf("[parent] pid=%d, child pid=%d\n", getpid(), pid);
    printf("[parent] NOT calling wait(). Sleep 30 seconds.\n");
    printf("Run in another terminal:\n");
    printf("  ps -o pid,ppid,stat,cmd -p %d -p %d\n", getpid(), pid);

    sleep(30);

    printf("[parent] done. exiting now.\n");
    return 0;
}
