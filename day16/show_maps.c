#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

int main(int argc, char **argv) {
    printf("[AFTER exec] pid=%d argc=%d\n", getpid(), argc);
    for (int i = 0; i < argc; i++) {
        printf("  argv[%d] = '%s'\n", i, argv[i]);
    }

    dump_maps_head("AFTER exec (new program)", 8);
    return 0;
}
