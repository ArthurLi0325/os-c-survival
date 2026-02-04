#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main(void) {
    const char *path = "day13/demo.txt";
    const size_t len = 4096; // 先用一頁大小，方便觀察

    // 1) open/create file
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) die("open");

    // 2) 先把檔案長度擴到 len，否則 mmap 寫入會 SIGBUS
    if (ftruncate(fd, (off_t)len) != 0) die("ftruncate");

    // 3) mmap file (shared)
    void *p = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (p == MAP_FAILED) die("mmap");

    close(fd); // mmap 完之後 fd 可關（mapping 還在）

    // 4) 寫入：改的是 memory，但 backing 是檔案
    const char *msg = "HELLO FROM MMAP FILE\n";
    memcpy(p, msg, strlen(msg));

    printf("[mmap_file] mapped %zu bytes at %p -> wrote message\n", len, p);
    printf("Now check file content in another terminal:\n");
    printf("  head -n 3 %s\n", path);
    printf("  hexdump -C %s | head\n", path);

    printf("\nPress Enter to msync+munmap and exit...\n");
    getchar();

    // 可選：強制同步到磁碟（MAP_SHARED 通常最後也會寫回，但這更明確）
    if (msync(p, len, MS_SYNC) != 0) die("msync");

    if (munmap(p, len) != 0) die("munmap");
    puts("[mmap_file] done");
    return 0;
}
