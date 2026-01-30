// day11/redirect_stdout.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

static void die(const char *msg){
    perror(msg);
    exit(1);
}

int main(void){
    // 1) 打開/建立 out.txt，並清空（truncate）
    int fd = open("day11/out.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0){
        die("open");
    }

    printf("open() returned fd=%d\n", fd);

    // 2) 把 stdout(1) 改接到 fd
    if (dup2(fd, STDOUT_FILENO) < 0){
        die("dup2");
    }

    // 3) fd 已經複製到 1 了，原本 fd 這個把手可以關掉
    close(fd);

    // 4) 之後所有寫 stdout 的輸出（printf/puts）都會進 out.txt
    printf("hello redirected stdout\n");
    printf("pid=%d\n", getpid());

    // 5) 保險起見，flush 一下 stdio buffer
    fflush(stdout);
    dprintf(STDOUT_FILENO, "write via fd=1 directly\n");

    return 0;
}