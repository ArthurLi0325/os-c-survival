#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

static long read_status_long(const char *path, const char *key) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char buf[4096];
    long val = -1;

    while (fgets(buf, sizeof(buf), f)) {
        size_t klen = strlen(key);
        if (strncmp(buf, key, klen) == 0) {
            // format examples:
            // VmRSS:      1408 kB
            // voluntary_ctxt_switches:        11
            char *p = buf + klen;
            while (*p == ' ' || *p == '\t') p++;
            errno = 0;
            val = strtol(p, NULL, 10);
            if (errno != 0) val = -1;
            break;
        }
    }

    fclose(f);
    return val;
}

static void print_rollup(const char *tag) {
    const char *path = "/proc/self/smaps_rollup";
    long rss = read_status_long(path, "Rss:");
    long pss = read_status_long(path, "Pss:");
    long sc  = read_status_long(path, "Shared_Clean:");
    long sd  = read_status_long(path, "Shared_Dirty:");
    long pc  = read_status_long(path, "Private_Clean:");
    long pd  = read_status_long(path, "Private_Dirty:");

    printf("[%s] rollup: Rss=%ld kB Pss=%ld kB  SC=%ld SD=%ld  PC=%ld PD=%ld\n",
           tag, rss, pss, sc, sd, pc, pd);
}

static void print_snapshot(const char *tag) {
    const char *path = "/proc/self/status";
    long vmrss = read_status_long(path, "VmRSS:");
    long vmsize = read_status_long(path, "VmSize:");
    long vol = read_status_long(path, "voluntary_ctxt_switches:");
    long invol = read_status_long(path, "nonvoluntary_ctxt_switches:");

    printf("[%s] pid=%d ppid=%d  VmSize=%ld kB  VmRSS=%ld kB  "
           "vol_cs=%ld  invol_cs=%ld\n",
           tag, getpid(), getppid(), vmsize, vmrss, vol, invol);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("=== proc_fork_compare (COW demo) ===\n");

    const size_t N = 32 * 1024 * 1024; // 32MB
    unsigned char *p = malloc(N);
    if (!p) {
        perror("malloc");
        return 1;
    }

    // touch pages: write one byte per 4096B page
    for (size_t i = 0; i < N; i += 4096) {
        p[i] = 1;
    }

    print_snapshot("parent:after_touch_before_fork");
    print_rollup ("parent:after_touch_before_fork");

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        print_snapshot("child:after_fork_before_write");
        print_rollup ("child:after_fork_before_write");

        // COW trigger: write again on each page
        for (size_t i = 0; i < N; i += 4096) {
            p[i] += 1;
        }

        print_snapshot("child:after_cow_write");
        print_rollup ("child:after_cow_write");
        sleep(30);
        _exit(0);
    }

    // parent
    print_snapshot("parent:after_fork");
    print_rollup ("parent:after_fork");
    sleep(30);

    int st = 0;
    waitpid(pid, &st, 0);

    print_snapshot("parent:after_wait");
    print_rollup ("parent:after_wait");
    printf("child exit status=%d\n", WIFEXITED(st) ? WEXITSTATUS(st) : -1);

    free(p);
    return 0;
}