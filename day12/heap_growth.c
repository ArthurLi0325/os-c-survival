// day12/heap_growth.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    printf("PID = %d\n", getpid());
    puts("Press Enter to start allocations...");
    getchar();

    void *p[20] = {0};

    for (int i = 0; i < 20; i++){
        p[i] = malloc(1024 * 1024); // 1 MiB each
        printf("i=%02d p=%p\n", i, p[i]);
        usleep(100 * 1000);
    }

    puts("Allocations done. Press Enter to exit...");
    getchar();

    for (int i = 0; i < 20; i++) free(p[i]);
    return 0;
}