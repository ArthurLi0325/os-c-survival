#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

static void reap_one_nonblock(pid_t target){
    int status;

    while (1) {
        pid_t w = waitpid(target, &status, WNOHANG);

        if (w == 0) {
            printf("[parent] pid=%d not ready, do other work...\n", target);
            usleep(200 * 1000); // 200ms
            continue;
        }

        if (w < 0) {
            perror("waitpid");
            exit(1);
        }

        printf("[parent] reaped pid=%d\n", w);

        if (WIFEXITED(status)){
            printf("[parent] exited, code=%d\n", WEXITSTATUS(status));
        }else if (WIFSIGNALED(status)){
            printf("[parent] signaled, sig=%d\n", WTERMSIG(status));
        }else{
            printf("[parent] other status\n");
        }

        break; //退出while(1)
    }
}

int main(void){
    pid_t c1 = fork();
    if (c1 < 0){
        perror("fork");
        exit(1);
    }

    if (c1 == 0){
        printf("[child1] pid=%d sleep 2 then exit(11)\n", getpid());
        sleep(2);
        exit(11);
    }

    pid_t c2 = fork();
    if (c2 < 0){
        perror("fork");
        exit(1);
    }

    if (c2 == 0){
        printf("[child2] pid=%d sleep 1 then exit(22)\n", getpid());
        sleep(1);
        exit(22);
    }

    printf("[parent] pid=%d, c1=%d, c2=%d\n", getpid(), c1, c2);

    int reaped = 0;
    while (reaped < 2) {
        int status;
        pid_t w = waitpid(-1, &status, WNOHANG);

        if (w == 0) {
            printf("[parent] no child exited yet, doing work...\n");
            usleep(200 * 1000);
            continue;
        }

        if (w < 0) {
            perror("waitpid");
            exit(1);
        }

        printf("[parent] reaped pid=%d\n", w);
        if (WIFEXITED(status)) {
            printf("[parent]   exited, code=%d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("[parent]   signaled, sig=%d\n", WTERMSIG(status));
        }

        reaped++;
    }

    printf("[parent] done\n");
    
    return 0;
}