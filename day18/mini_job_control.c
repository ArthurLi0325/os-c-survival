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

int main(void) {
    // parent/shell 行為：避免背景碰 terminal 時被 SIGTTOU/SIGTTIN 停住
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    pid_t shell_pgid = getpgrp();
    pid_t fg_pgid = tcgetpgrp(STDIN_FILENO);
    printf("[parent] pid=%d pgid=%d tcgetpgrp=%d\n", getpid(), getpgrp(), fg_pgid);

    pid_t pid = fork();
    if (pid < 0) die("fork");

    if (pid == 0) {
        // child：自成一個 process group
        if (setpgid(0, 0) < 0) die("child setpgid");

        // child 要吃鍵盤訊號（Ctrl+Z / Ctrl+C 等）→ 還原為預設
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGINT,  SIG_DFL);
        signal(SIGQUIT, SIG_DFL);

        printf("[child ] pid=%d pgid=%d (running... Ctrl+Z to stop)\n", getpid(), getpgrp());
        fflush(stdout);

        for (int i = 0; ; i++) {
            printf("[child ] tick %d\n", i);
            fflush(stdout);
            sleep(1);
        }
        _exit(0);
    }

    // parent：也做一次 setpgid 避免 race
    if (setpgid(pid, pid) < 0) {
        if (errno != EACCES) die("parent setpgid");
    }

    printf("[parent] give terminal to child pgid=%d\n", pid);
    if (tcsetpgrp(STDIN_FILENO, pid) < 0) die("tcsetpgrp to child");

    // 等第一次 stop（Ctrl+Z），用 WUNTRACED 才會收到 STOP 狀態
    int status = 0;
    pid_t w = waitpid(pid, &status, WUNTRACED);
    if (w < 0) die("waitpid WUNTRACED");

    if (WIFSTOPPED(status)) {
        printf("[parent] child stopped by signal %d\n", WSTOPSIG(status));
    } else if (WIFEXITED(status)) {
        printf("[parent] child exited code=%d\n", WEXITSTATUS(status));
        tcsetpgrp(STDIN_FILENO, shell_pgid);
        return 0;
    } else if (WIFSIGNALED(status)) {
        printf("[parent] child killed by signal %d\n", WTERMSIG(status));
        tcsetpgrp(STDIN_FILENO, shell_pgid);
        return 0;
    } else {
        printf("[parent] unexpected: status=0x%x\n", status);
    }

    // terminal 還給 parent/shell，才能讀你的輸入（模擬 shell）
    printf("[parent] restore terminal to shell_pgid=%d\n", shell_pgid);
    if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0) die("tcsetpgrp to shell");

    // ===== 互動：fg / kill / exit =====
    while (1) {
        char buf[64];

        printf("[parent] command? (fg / kill / exit) > ");
        fflush(stdout);

        if (!fgets(buf, sizeof(buf), stdin)) {
            printf("\n[parent] EOF\n");
            break;
        }

        if (buf[0] == 'f') {  // fg
            printf("[parent] fg: SIGCONT to pgid=%d\n", pid);
            fflush(stdout);

            if (kill(-pid, SIGCONT) < 0) die("kill SIGCONT");

            if (tcsetpgrp(STDIN_FILENO, pid) < 0)
                die("tcsetpgrp to child (fg)");

            // 等到它「再次 stop」或「結束」
            pid_t w2 = waitpid(pid, &status, WUNTRACED);
            if (w2 < 0) die("waitpid after fg");

            if (WIFSTOPPED(status)) {
                printf("[parent] child stopped again by signal %d\n", WSTOPSIG(status));

                printf("[parent] restore terminal to shell_pgid=%d\n", shell_pgid);
                if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0)
                    die("tcsetpgrp restore shell");
                continue;
            }

            if (WIFEXITED(status)) {
                printf("[parent] child exited code=%d\n", WEXITSTATUS(status));
                break;
            }

            if (WIFSIGNALED(status)) {
                printf("[parent] child killed by signal %d\n", WTERMSIG(status));
                break;
            }

            printf("[parent] child state changed (status=0x%x)\n", status);
            break;

        } else if (buf[0] == 'k') {  // kill
            printf("[parent] kill: SIGTERM to pgid=%d (ensure continued)\n", pid);
            fflush(stdout);

            // terminal 先回到 parent/shell
            if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0)
                die("tcsetpgrp restore before kill");

            // 關鍵：stopped job 先 SIGCONT，確保後續 SIGTERM 能推進到「結束」
            if (kill(-pid, SIGCONT) < 0)
                die("kill SIGCONT (before SIGTERM)");

            if (kill(-pid, SIGTERM) < 0)
                die("kill SIGTERM");

            // 回收這個 pgid 裡的子行程（就算只有一個也 OK）
            while (1) {
                pid_t r = waitpid(-pid, &status, 0); // -pid = wait any child in that pgid
                if (r > 0) {
                    if (WIFSIGNALED(status))
                        printf("[parent] reaped pid=%d killed by signal %d\n", r, WTERMSIG(status));
                    else if (WIFEXITED(status))
                        printf("[parent] reaped pid=%d exited code=%d\n", r, WEXITSTATUS(status));
                    else
                        printf("[parent] reaped pid=%d status=0x%x\n", r, status);
                    continue;
                }
                if (r == -1 && errno == EINTR) continue;
                if (r == -1 && errno == ECHILD) break;
                die("waitpid(-pgid) after kill");
            }

            break;

        } else if (buf[0] == 'e') {  // exit
            printf("[parent] exit: leaving child stopped\n");
            break;

        } else {
            printf("[parent] unknown command: %s", buf);
        }
    }

    // 保險：結束前把 terminal 還給 shell/parent
    if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0) die("tcsetpgrp final restore");

    return 0;
}
