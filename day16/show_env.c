#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const char *a = getenv("DAY16_A");
    const char *b = getenv("DAY16_B");
    const char *path = getenv("PATH");

    printf("[show_env] DAY16_A=%s\n", a ? a : "(null)");
    printf("[show_env] DAY16_B=%s\n", b ? b : "(null)");
    printf("[show_env] PATH=%s\n", path ? path : "(null)");

    return 0;
}