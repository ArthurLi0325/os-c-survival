#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    printf("Before exec: PID=%d\n", getpid());

    execl("/bin/ls", "ls", "-l", NULL);

    printf("This will NOT print\n");
    return 0;
}
