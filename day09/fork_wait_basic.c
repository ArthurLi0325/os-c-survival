#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void){
    setvbuf(stdout, NULL, _IONBF, 0);

    pid_t pid = fork();

    if (pid < 0){
        perror("fork");
        exit(1);
    }

    if (pid == 0){
        // child
        printf("[child] pid=%d, ppid=%d\n", getpid(), getppid());
        sleep(1);
        printf("[child] exit now\n");
        exit(0);
    }else{
        // parent
        printf("[parent] pid=%d, child pid=%d\n", getpid(), pid);

        printf("[parent] wait()...\n");
        wait(NULL);
        printf("[parent] child reaped, continue\n");

        exit(0);
    }
}