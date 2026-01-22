// day10/pipe_fork.c
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void die(const char *msg){
    perror(msg);
    exit(1);
}

int main(void){
    int fd[2];

    // 1) 建立 pipe：fd[0] 讀端, fd[1] 寫端
    if (pipe(fd) == -1){
        die("pipe");
    }
    // 2) fork：複製目前 process（包含 fd table）
    pid_t pid = fork();
    if (pid < 0){
        die("fork");
    }

    if (pid == 0){
        // ---------------- child process ----------------

        // child 只讀，所以先關掉寫端
        if (close(fd[1]) == -1){
            die("close write end in child");
        }
        // 讀 pipe 資料（阻塞直到：有資料 或 對端全部關閉寫端 => EOF
        char buf[128];
        ssize_t n = read(fd[0], buf, sizeof(buf) - 1);
        if (n < 0){
            die("read");
        }
        // 讓 buf 變成可印出的 C 字串
        buf[n] = '\0';

        printf("[child] got %zd bytes: %s", n, buf);
        // 讀完就關讀端
        if (close(fd[0]) == -1){
            die("close read end in child");
        }
        _exit(0);
    }else{
        // ---------------- parent process ----------------

        // parent 只寫，所以先關掉讀端
        if (close(fd[0]) == -1){
            die("close read end in parent");
        }

        const char *msg = "hello from parent\n";
        ssize_t len = (ssize_t)strlen(msg);
        // 寫入 pipe（寫端仍開著，child 才能讀到資料）
        ssize_t n = write(fd[1], msg, (size_t)len);
        if (n < 0){
            die("write");
        }

        printf("[parent] wrote %zd bytes\n", n);
        // 關掉寫端：很重要，讓 child 最終能看到 EOF
        if (close(fd[1]) == -1){
            die("close write end in parent");
        }

        int status = 0;
        pid_t w = waitpid(pid, &status, 0);
        if (w < 0){
            die("waitpid");
        }
        if (WIFEXITED(status)) {
            printf("[parent] child exit code=%d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("[parent] child killed by signal=%d\n", WTERMSIG(status));
        } else {
            printf("[parent] child ended unexpectedly\n");
        }

        return 0;
    }
}