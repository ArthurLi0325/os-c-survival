#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int want_key(const char *line) {
    static const char *keys[] = {
        "Name:",
        "State:",
        "Tgid:",
        "Pid:",
        "PPid:",
        "Threads:",
        "VmSize:",
        "VmRSS:",
        "voluntary_ctxt_switches:",
        "nonvoluntary_ctxt_switches:",
    };

    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        size_t klen = strlen(keys[i]);
        if (strncmp(line, keys[i], klen) == 0) return 1;
    }
    return 0;
}

static void usage(const char *argv0) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s              # inspect self\n", argv0);
    fprintf(stderr, "  %s --pid <pid>   # inspect target pid\n", argv0);
}

int main(int argc, char **argv) {
    const char *path = "/proc/self/status";
    char path_buf[128];

    if (argc == 1) {
        // default: self
    } else if (argc == 3 && strcmp(argv[1], "--pid") == 0) {
        char *end = NULL;
        long pid = strtol(argv[2], &end, 10);
        if (!end || *end != '\0' || pid <= 0) {
            fprintf(stderr, "Invalid pid: %s\n", argv[2]);
            usage(argv[0]);
            return 2;
        }
        snprintf(path_buf, sizeof(path_buf), "/proc/%ld/status", pid);
        path = path_buf;
    } else {
        usage(argv[0]);
        return 2;
    }

    printf("=== proc_inspect ===\n");
    printf("getpid()=%d\n", getpid());
    printf("target=%s\n", path);

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "fopen %s failed: %s\n", path, strerror(errno));
        return 1;
    }

    char buf[4096];
    while (fgets(buf, sizeof(buf), f)) {
        if (want_key(buf)) fputs(buf, stdout);
    }

    fclose(f);
    return 0;
}
