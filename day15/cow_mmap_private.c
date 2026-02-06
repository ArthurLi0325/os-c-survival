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

    // MAP_PRIVATE：fork 後「讀」可共享，但「寫」會觸發 COW 分裂成 private page
    int *p = mmap(NULL, page, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) die("mmap");

    *p = 123;
    printf("[before fork] pid=%d, p=%p, *p=%d (MAP_PRIVATE|ANON)\n",
           getpid(), (void*)p, *p);

    pid_t c = fork();
    if (c < 0) die("fork");

    if (c == 0) {
        // child
        printf("[child]  pid=%d, p=%p, *p(before)=%d\n",
               getpid(), (void*)p, *p);

        // 讓你有時間在另一個 terminal 觀察 /proc/<pid>/smaps
        printf("[child]  pause 30s (check smaps now)\n");
        sleep(30);

        // 觸發 COW：第一次寫入 MAP_PRIVATE 對應的頁
        *p = 999;
        printf("[child]  wrote *p=999\n");

        // 再留時間讓你看 smaps 寫入後的變化
        printf("[child]  pause 30s (check smaps after write)\n");
        sleep(30);

        _exit(0);
    }

    // parent
    printf("[parent] pid=%d, child=%d, p=%p, *p(initial)=%d\n",
           getpid(), c, (void*)p, *p);

    // parent 也留時間給你看 child 寫前的狀態
    printf("[parent] pause 30s (check child smaps before child write)\n");
    sleep(30);

    // 等 child 寫完後，再讀一次 *p
    printf("[parent] after child write, *p=%d (should still be 123)\n", *p);

    int st = 0;
    waitpid(c, &st, 0);

    munmap(p, page);
    return 0;
}
