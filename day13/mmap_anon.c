#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main(void) {
    long page = sysconf(_SC_PAGESIZE);
    if (page <= 0) die("sysconf");

    size_t len = (size_t)page; // 先只配 1 page，最小可驗證

    // 匿名 mmap：不對應任何檔案
    void *p = mmap(NULL, len,
                   PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS,
                   -1, 0);
    if (p == MAP_FAILED) die("mmap");

    printf("[mmap] page=%ld bytes, p=%p\n", page, p);

    // 寫入幾個 bytes，確保真的用到那頁
    strcpy((char *)p, "hello mmap\n");
    printf("[mmap] content: %s", (char *)p);

    // 提示你用另一個 terminal 去看 maps
    printf("\nNow run in another terminal:\n");
    printf("  cat /proc/%d/maps | grep -E \"mmap_anon|\\[heap\\]|\\[stack\\]\" -n\n", getpid());
    printf("Or just:\n");
    printf("  cat /proc/%d/maps\n", getpid());

    // 停住讓你有時間觀察
    printf("\nPress Enter to munmap and exit...\n");
    getchar();

    if (munmap(p, len) != 0) die("munmap");

    puts("[mmap] munmap done, bye");
    return 0;
}
