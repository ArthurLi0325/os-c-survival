// day11/pipe_two_cmd.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main(void) {
    int p[2];
    if (pipe(p) < 0) die("pipe");

    pid_t c1 = fork();
    if (c1 < 0) die("fork c1");

    if (c1 == 0) {
        // child1: ls  (stdout -> pipe write)
        if (dup2(p[1], STDOUT_FILENO) < 0) die("dup2 c1");
        close(p[0]); // child1 不讀
        close(p[1]); // dup2 後原把手可關

        execlp("ls", "ls", "day11", (char *)NULL);
        die("execlp ls");
    }

    pid_t c2 = fork();
    if (c2 < 0) die("fork c2");

    if (c2 == 0) {
        // child2: wc -l  (stdin <- pipe read)
        if (dup2(p[0], STDIN_FILENO) < 0) die("dup2 c2");
        close(p[0]);
        close(p[1]); // child2 不寫

        execlp("wc", "wc", "-l", (char *)NULL);
        die("execlp wc");
    }

    // parent: 自己不用 pipe，必須關掉兩端，否則 wc 可能等不到 EOF

    close(p[0]);
    close(p[1]);

    int st1, st2;
    waitpid(c1, &st1, 0);
    waitpid(c2, &st2, 0);

    return 0;
}