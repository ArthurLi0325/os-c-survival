#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

static void dump_maps_head(const char *tag, int max_lines) {
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) {
        perror("fopen /proc/self/maps");
        exit(1);
    }

    printf("=== %s: PID=%d /proc/self/maps (first %d lines) ===\n",
           tag, getpid(), max_lines);

    char buf[4096];
    int n = 0;
    while (n < max_lines && fgets(buf, sizeof(buf), f)) {
        fputs(buf, stdout);
        n++;
    }
    fclose(f);
    printf("=== end ===\n");
}

int main(void) {
    printf("[before exec] pid=%d\n", getpid());
    dump_maps_head("BEFORE exec (old program)", 12);

    printf("[before exec] now exec ./day16/show_maps (PID stays same)\n");
    execl("./day16/show_maps", "show_maps", "AAA", "BBB", NULL);

    perror("execl failed");
    return 1;
}
