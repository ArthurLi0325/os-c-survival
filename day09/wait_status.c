#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        // child
        printf("[child] pid=%d\n", getpid());
        sleep(1);
        exit(42);   // ← 先從這個開始
    }

    // parent
    int status;
    pid_t w = wait(&status);

    printf("[parent] wait returned pid=%d\n", w);

    if (WIFEXITED(status)){
        printf("[parent] child exited normally\n");
        printf("[parent] exit code = %d\n", WEXITSTATUS(status));
    }

    if (WIFSIGNALED(status)){
        printf("[parent] child killed by signal %d\n", WTERMSIG(status));
    }

    return 0;
}