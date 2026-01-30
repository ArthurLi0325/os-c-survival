// day11/redirect_stdin.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main(void){
    // 1) open: 把檔案打開成「可讀」
    int fd = open("day11/input.txt", O_RDONLY);
    if (fd < 0){
        die("open");
    }

    // 2) dup2: 把 stdin(0) 改接到 fd
    if (dup2(fd, STDIN_FILENO) < 0){
        die("dup2");
    }

    // 3) close: fd 已經複製到 0 了，原 fd 把手不再需要
    close(fd);

    // 4) 用 stdio 從 stdin 讀（現在其實在讀檔案）
    char buf[128];

    //while (fgets(buf, sizeof(buf), stdin)){
    //    printf("read: %s", buf);
    //}

    ssize_t n;
    while ((n = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        write(STDOUT_FILENO, "chunk: ", 7);
        write(STDOUT_FILENO, buf, n);
    }

    return 0;

}