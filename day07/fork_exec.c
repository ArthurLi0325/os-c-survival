#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void){
    printf("[parent] start pid=%d\n", getpid());

    pid_t pid = fork();
    if (pid < 0){
        perror("fork");
        return 1;
    }

    if (pid == 0){
        // child
        printf("[child] before exec pid=%d\n", getpid());

        execl("/bin/ls", "ls", "-1", "day07", NULL);

        // 只有 exec 失敗才會跑到這裡
        perror("execl");
        exit(1);
    }else{
        int status = 0;
        waitpid(pid, &status, 0);
        printf("[parent] child done, pid=%d, status=%d\n", pid, status);
    }

    return 0;
}