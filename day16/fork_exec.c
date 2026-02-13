#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        printf("[child] pid=%d, going to exec\n", getpid());
        execl("/bin/ls", "ls", NULL);
        perror("exec failed");
        exit(1);
    } else {
        printf("[parent] pid=%d, child=%d\n", getpid(), pid);
        wait(NULL);
        printf("[parent] child finished\n");
    }

    return 0;
}
