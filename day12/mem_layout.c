// day12/mem_layout.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int g_init = 42;        // .data (initialized global)
int g_bss;              // .bss  (uninitialized global)

static int s_init = 7;  // .data (initialized static)
static int s_bss;       // .bss  (uninitialized static)

int main(void) {
    int local = 123;                 // stack
    static int ls_init = 9;          // .data (static local)
    static int ls_bss;               // .bss  (static local)
    void *heap_p = malloc(16);       // heap (likely)
    const char *str = "hello";       // string literal in rodata

    printf("PID = %d\n", getpid());

    printf("&g_init   = %p\n", (void*)&g_init);
    printf("&g_bss    = %p\n", (void*)&g_bss);
    printf("&s_init   = %p\n", (void*)&s_init);
    printf("&s_bss    = %p\n", (void*)&s_bss);

    printf("&ls_init  = %p\n", (void*)&ls_init);
    printf("&ls_bss   = %p\n", (void*)&ls_bss);

    printf("&local    = %p\n", (void*)&local);
    printf("heap_p    = %p\n", heap_p);
    printf("str       = %p\n", (void*)str);

    puts("Press Enter to keep the process alive...");
    getchar();

    free(heap_p);
    return 0;
}