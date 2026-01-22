// day10/pipe_exec.c
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main(void) {
    int fd[2];

    // 1) 建立 pipe
    if (pipe(fd) == -1) die("pipe");

    // 2) fork 第一個 child：負責 ls
    pid_t pid1 = fork();
    if (pid1 < 0) die("fork ls");

    if (pid1 == 0) {
        // ---------- child 1: ls ----------

        // ls 要寫到 pipe，所以：
        // - stdout -> pipe write end
        if (dup2(fd[1], STDOUT_FILENO) == -1)
            die("dup2 ls stdout");

        // 關掉不用的 fd（dup2 完後一定要關）
        close(fd[0]); // 不讀
        close(fd[1]); // 已經 dup 到 stdout

        execlp("ls", "ls", NULL);
        die("execlp ls");
    }

    // 3) fork 第二個 child：負責 wc -l
    pid_t pid2 = fork();
    if (pid2 < 0) die("fork wc");

    if (pid2 == 0) {
        // ---------- child 2: wc -l ----------

        // wc 要從 pipe 讀，所以：
        // - stdin <- pipe read end
        if (dup2(fd[0], STDIN_FILENO) == -1)
            die("dup2 wc stdin");

        close(fd[0]); // 已經 dup 到 stdin
        close(fd[1]); // 不寫

        execlp("wc", "wc", "-l", NULL);
        die("execlp wc");
    }

    // ---------- parent ----------

    // parent 不用 pipe，全部關掉
    close(fd[0]);
    close(fd[1]);

    // 等兩個 child
    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);

    return 0;
}