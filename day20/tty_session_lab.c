#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

static int open_tty_rd(void) {
    int fd = open("/dev/tty", O_RDONLY);
    return fd;
}

static int open_tty_wr(void) {
    int fd = open("/dev/tty", O_WRONLY);
    return fd;
}

static pid_t get_fg_pgid(void) {
    int fd = open_tty_rd();
    if (fd < 0) return -1;
    pid_t fg = tcgetpgrp(fd);
    close(fd);
    return fg;
}

static void set_fg_pgid(pid_t pgid) {
    int fd = open_tty_wr();
    if (fd < 0) die("open(/dev/tty)");
    if (tcsetpgrp(fd, pgid) < 0) die("tcsetpgrp");
    close(fd);
}

static void print_ids(const char *tag) {
    pid_t pid  = getpid();
    pid_t ppid = getppid();
    pid_t pgid = getpgrp();
    pid_t sid  = getsid(0);

    int in_tty  = isatty(STDIN_FILENO);
    int out_tty = isatty(STDOUT_FILENO);
    int err_tty = isatty(STDERR_FILENO);

    pid_t fg = get_fg_pgid();

    printf("=== %s ===\n", tag);
    printf("pid=%d ppid=%d pgid=%d sid=%d\n", pid, ppid, pgid, sid);
    printf("isatty: stdin=%d stdout=%d stderr=%d\n", in_tty, out_tty, err_tty);
    if (fg >= 0) printf("tcgetpgrp(/dev/tty)=%d\n", fg);
    else         printf("tcgetpgrp(/dev/tty)=<no tty>\n");
    fflush(stdout);
}

static void child_work(void) {
    // child 這裡故意做「會一直輸出」的工作，方便你按 Ctrl+Z / Ctrl+C 觀察
    for (int i = 0; i < 1000; i++) {
        printf("[child] tick %d (pid=%d pgid=%d fg=%d)\n",
               i, getpid(), getpgrp(), get_fg_pgid());
        fflush(stdout);
        sleep(1);
    }
}

int main(void) {
    // 很重要：避免 parent 在做 tcsetpgrp 時，被 kernel 因為「背景寫 tty」而送 SIGTTOU 停掉
    signal(SIGTTOU, SIG_IGN);

    print_ids("parent: start (before fork)");

    pid_t shell_fg = get_fg_pgid();   // 程式啟動時，terminal 前景 pgid（之後要還回去）
    if (shell_fg < 0) die("get_fg_pgid");

    pid_t pid = fork();
    if (pid < 0) die("fork");

    if (pid == 0) {
        // ===== child =====
        // 讓 child 自己當一個新的 process group leader（pgid = pid）
        if (setpgid(0, 0) < 0) die("child setpgid(0,0)");

        // child 最好恢復預設行為，讓 Ctrl+C / Ctrl+Z 的效果更直觀
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);

        print_ids("child: after setpgid");
        child_work();
        _exit(0);
    }

    // ===== parent =====
    // 保險：parent 也嘗試把 child 放進自己的新 PG（避免競態）
    if (setpgid(pid, pid) < 0 && errno != EACCES) {
        // EACCES 有時是 child 已經 setpgid 成功，屬於可接受競態
        die("parent setpgid(child,child)");
    }

    printf("[parent] spawned child pid=%d, will give terminal to child pgid=%d\n", pid, pid);
    fflush(stdout);

    // 第一次把 terminal 給 child
    set_fg_pgid(pid);

    while (1) {
        int status;
        pid_t w = waitpid(pid, &status, WUNTRACED);
        if (w < 0) die("waitpid");

        if (WIFSTOPPED(status)) {
            printf("[parent] child stopped by signal %d\n", WSTOPSIG(status));
            fflush(stdout);

            // 把 terminal 還給 shell(本程式的 pg)
            set_fg_pgid(shell_fg);
            printf("[parent] terminal restored to shell_fg=%d\n", shell_fg);
            fflush(stdout);

            // 自動做 fg：SIGCONT + 把 terminal 再交給 child
            printf("[parent] auto-fg: SIGCONT child and give terminal back to it\n");
            fflush(stdout);

            kill(-pid, SIGCONT);
            set_fg_pgid(pid);
            continue;
        }

        // child 結束或被殺死：收尾 + break
        set_fg_pgid(shell_fg);

        if (WIFSIGNALED(status)) {
            printf("[parent] child killed by signal %d\n", WTERMSIG(status));
        } else if (WIFEXITED(status)) {
            printf("[parent] child exited code=%d\n", WEXITSTATUS(status));
        } else {
            printf("[parent] child ended (status=0x%x)\n", status);
        }
        fflush(stdout);
        break;
    }

    return 0;
}
