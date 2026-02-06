#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main(void) {
    size_t page = (size_t)sysconf(_SC_PAGESIZE);
    if (page == 0) die("sysconf");

    int *p = mmap(NULL, page, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) die("mmap");

    *p = 123;
    printf("[before fork] pid=%d, p=%p, *p=%d\n",
           getpid(), (void*)p, *p);

    int fds[2];
    if (pipe(fds) < 0) die("pipe");
    // fds[0] = read end, fds[1] = write end

    pid_t c = fork();
    if (c < 0) die("fork");

    if (c == 0) {
        // ===== child =====
        close(fds[0]); // child 不讀 pipe

        printf("[child]  pid=%d, *p(before)=%d\n", getpid(), *p);
        sleep(30);

        *p = 999;  // 觸發 COW
        printf("[child]  wrote *p=999\n");

        // 通知 parent：我寫完了
        if (write(fds[1], "X", 1) != 1)
            die("child write pipe");

        sleep(30); // 留時間給你看 smaps（寫入後）
        _exit(0);
    }

    // ===== parent =====
    close(fds[1]); // parent 不寫 pipe

    printf("[parent] pid=%d waiting for child...\n", getpid());

    char buf;
    if (read(fds[0], &buf, 1) != 1)
        die("parent read pipe");

    // 一定在 child 寫完 *p=999 之後才會到這裡
    printf("[parent] child done, *p=%d (must be 123)\n", *p);

    waitpid(c, NULL, 0);
    munmap(p, page);
    return 0;
}
