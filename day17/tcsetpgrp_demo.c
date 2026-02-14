#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main() {
    // 讓 parent 在嘗試寫 terminal 時不會被 SIGTTOU 暫停
    signal(SIGTTOU, SIG_IGN);

    int ttyfd = STDIN_FILENO; // 控制終端通常是 stdin
    pid_t shell_pgid = tcgetpgrp(ttyfd);
    if (shell_pgid < 0) die("tcgetpgrp");

    printf("[parent] pid=%d pgid=%d shell_fg_pgid=%d\n",
           getpid(), getpgid(0), shell_pgid);
    fflush(stdout);

    pid_t pid = fork();
    if (pid < 0) die("fork");

    if (pid == 0) {
        // child 自己當 leader：PGID = PID
        if (setpgid(0, 0) < 0) die("child setpgid");

        printf("[child ] pid=%d pgid=%d (waiting... try Ctrl+C)\n",
               getpid(), getpgid(0));
        fflush(stdout);

        while (1) sleep(1);
    }

    // parent：確保 child 的 pgid 設好（避免 race）
    if (setpgid(pid, pid) < 0 && errno != EACCES) die("parent setpgid");

    // 把前景切給 child 的 process group
    printf("[parent] give terminal to child pgid=%d\n", pid);
    fflush(stdout);
    if (tcsetpgrp(ttyfd, pid) < 0) die("tcsetpgrp to child");

    // 等 child 結束（你按 Ctrl+C 應該只會殺 child）
    int status;
    if (waitpid(pid, &status, 0) < 0) die("waitpid");

    // 把前景拿回來（通常拿回 shell 那個 PGID）
    printf("[parent] child exited, restore terminal to shell_fg_pgid=%d\n", shell_pgid);
    fflush(stdout);
    if (tcsetpgrp(ttyfd, shell_pgid) < 0) die("tcsetpgrp restore");

    if (WIFSIGNALED(status)) {
        printf("[parent] child died by signal %d\n", WTERMSIG(status));
    } else if (WIFEXITED(status)) {
        printf("[parent] child exited code=%d\n", WEXITSTATUS(status));
    }
    return 0;
}