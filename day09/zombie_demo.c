#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void){
    pid_t pid = fork();
    if (pid < 0){
        perror("fork");
        exit(1);
    }

    if (pid == 0){
        printf("[child] pid=%d, exiting immediately\n", getpid());
        exit(0);
    }

    printf("[parent] pid=%d, child pid=%d\n", getpid(), pid);
    printf("[parent] NOT calling wait(). Sleep 30s...\n");
    sleep(30);
    printf("[parent] done\n");
    return 0;
}