// day08/zombie_reap.c
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        printf("[child] pid=%d: exiting now\n", getpid());
        _exit(42);
    }

    // parent
    printf("[parent] pid=%d, child pid=%d\n", getpid(), pid);
    printf("[parent] child should become zombie until we reap it.\n");
    printf("In another terminal, run:\n");
    printf("  ps -o pid,ppid,stat,cmd -p %d -p %d\n", getpid(), pid);
    printf("\nPress Enter to reap child via waitpid()...\n");

    // 等你按 Enter
    getchar();

    int status;
    pid_t w = waitpid(pid, &status, 0);
    if (w < 0) {
        printf("[parent] waitpid error: errno=%d (%s)\n", errno, strerror(errno));
        return 1;
    }

    if (WIFEXITED(status)) {
        printf("[parent] reaped child %d, exit=%d\n", w, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("[parent] reaped child %d, killed by signal=%d\n", w, WTERMSIG(status));
    } else {
        printf("[parent] reaped child %d, status=0x%x\n", w, status);
    }

    printf("[parent] sleeping 10s so you can re-check ps (child should be gone)\n");
    sleep(10);

    return 0;
}
