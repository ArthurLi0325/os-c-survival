// day11/pipe_three_cmd.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main(void) {
    int p1[2]; // ls -> grep
    int p2[2]; // grep -> wc

    if (pipe(p1) < 0) die("pipe p1");
    if (pipe(p2) < 0) die("pipe p2");

    // --- child1: ls day11  (stdout -> p1 write) ---
    pid_t c1 = fork();
    if (c1 < 0) die("fork c1");
    if (c1 == 0) {
        if (dup2(p1[1], STDOUT_FILENO) < 0) die("dup2 c1");
        // close all pipe fds (after dup2)
        close(p1[0]); close(p1[1]);
        close(p2[0]); close(p2[1]);
        
        dprintf(2, "[c1] pid=%d\n", getpid());

        execlp("ls", "ls", "day11", (char *)NULL);
        die("execlp ls");
    }

    // --- child2: grep c  (stdin <- p1 read, stdout -> p2 write) ---
    pid_t c2 = fork();
    if (c2 < 0) die("fork c2");
    if (c2 == 0) {
        if (dup2(p1[0], STDIN_FILENO) < 0) die("dup2 c2 stdin");
        if (dup2(p2[1], STDOUT_FILENO) < 0) die("dup2 c2 stdout");
        close(p1[0]); close(p1[1]);
        close(p2[0]);
        // close(p2[1]);  // 故意不關

        dprintf(2, "[c2] pid=%d\n", getpid());

        execlp("grep", "grep", "c", (char *)NULL);
        die("execlp grep");
    }

    // --- child3: wc -l  (stdin <- p2 read) ---
    pid_t c3 = fork();
    if (c3 < 0) die("fork c3");
    if (c3 == 0) {
        if (dup2(p2[0], STDIN_FILENO) < 0) die("dup2 c3");
        close(p1[0]); close(p1[1]);
        close(p2[0]); close(p2[1]);

        dprintf(2, "[c3] pid=%d\n", getpid());

        execlp("wc", "wc", "-l", (char *)NULL);
        die("execlp wc");
    }
    
    dprintf(2, "[parent] closing p1[0],p1[1],p2[0],p2[1]\n");
    close(p1[0]); close(p1[1]);
    close(p2[0]); close(p2[1]);

    int st;
    waitpid(c1, &st, 0);
    waitpid(c2, &st, 0);
    waitpid(c3, &st, 0);

    return 0;
}