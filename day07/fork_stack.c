#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(void){
    int x = 10;

    printf("[before fork] pid=%d, x=%d, &x=%p\n", getpid(), x, (void*)&x);

    pid_t pid = fork();
    if (pid < 0){
        perror("fork");
        return 1;
    }

    // 讓 parent/child 的輸出比較不會互相插隊
    usleep(100 * 1000);

    if (pid == 0){
        // child
        x++;
        printf("[child ] pid=%d, ppid=%d, x=%d, &x=%p\n", getpid(), getppid(), x, (void*)&x);
    }else{
        // parent
        x += 100;
        printf("[parent] pid=%d, child=%d, x=%d, &x=%p\n", getpid(), pid, x, (void*)&x);
    }

    return 0;
}