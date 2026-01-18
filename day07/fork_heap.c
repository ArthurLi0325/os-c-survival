#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(void){
    int *p = malloc(sizeof(int));
    if (!p){
        perror("malloc");
        return 1;
    }
    *p = 5;

    printf("[before fork] pid=%d, p=%p, *p=%d\n", getpid(), (void*)p, *p);

    pid_t pid = fork();
    if (pid < 0){
        perror("fork");
        return 1;
    }

    usleep(100 * 1000);

    if (pid == 0){
        // child
        *p = 999;
        printf("[child ] pid=%d, p=%p, *p=%d\n", getpid(), (void*)p, *p);
    }else{
        // parent
        *p = 111;
        printf("[parent] pid=%d, p=%p, *p=%d\n", getpid(), (void*)p, *p);
    }

    free(p);
    return 0;
}