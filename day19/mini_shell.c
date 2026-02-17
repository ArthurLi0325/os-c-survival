#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE 1024
#define MAX_ARGS 64

static int parse_line(char *line, char *argv[], int max_args) {
    // 去掉結尾的 '\n'
    size_t n = strlen(line);
    if (n > 0 && line[n - 1] == '\n') line[n - 1] = '\0';

    int argc = 0;
    char *save = NULL;
    for (char *tok = strtok_r(line, " \t", &save);
         tok != NULL && argc < max_args - 1;
         tok = strtok_r(NULL, " \t", &save)) {
        argv[argc++] = tok;
    }
    argv[argc] = NULL;
    return argc;
}

int main(void) {
    char line[MAX_LINE];
    pid_t shell_pgid = getpgrp();
    pid_t last_stopped_pgid = -1;

    signal(SIGINT,  SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    while (1) {
        // reap any finished children (avoid zombies)
        int status;
        pid_t w;
        while ((w = waitpid(-1, &status, WNOHANG)) > 0) {
            printf("[reaped] pid=%d\n", w);
        }

        // prompt
        printf("mini$ ");
        fflush(stdout);

        // EOF (Ctrl+D) -> exit
        if (!fgets(line, sizeof(line), stdin)) {
            putchar('\n');
            break;
        }

        // 解析參數
        char *argv[MAX_ARGS];
        int argc = parse_line(line, argv, MAX_ARGS);
        if (argc == 0) continue;

        int bg = 0;
        if (argc > 0 && strcmp(argv[argc - 1], "&") == 0) {
            bg = 1;
            argv[argc - 1] = NULL;
            argc--;
            if (argc == 0) continue; // 只有輸入 "&" 的情況
        }

        // built-in: exit
        if (strcmp(argv[0], "exit") == 0) break;

        if (strcmp(argv[0], "fg") == 0) {
            if (last_stopped_pgid < 0) {
                printf("fg: no stopped job\n");
                continue;
            }

            // give terminal to that pgid
            if (tcsetpgrp(STDIN_FILENO, last_stopped_pgid) < 0) {
                perror("tcsetpgrp to job");
                continue;
            }

            // continue the job
            if (kill(-last_stopped_pgid, SIGCONT) < 0) {
                printf("[fg] pgid=%d\n", last_stopped_pgid);
                perror("kill(SIGCONT)");
            }

            int fg_status;
            if (waitpid(-last_stopped_pgid, &fg_status, WUNTRACED) < 0) {
                perror("waitpid(fg)");
            }

            // restore terminal
            if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0) {
                perror("tcsetpgrp to shell");
            }

            if (!WIFSTOPPED(fg_status)) {
                last_stopped_pgid = -1; // 結束了就清掉
            }
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            // child
            // put child in its own process group
            setpgid(0, 0);
            // child should receive Ctrl+C / Ctrl+Z normally
            signal(SIGINT,  SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);

            execvp(argv[0], argv);
            // exec 失敗才會到這裡
            perror("execvp");
            _exit(127);
        }

        // set child's process group (race-safe)
        setpgid(pid, pid);

        // parent: wait child
        if (!bg) {
            // give terminal to child pgid
            if (tcsetpgrp(STDIN_FILENO, pid) < 0) {
                perror("tcsetpgrp to child");
            }

            int fg_status;
            if (waitpid(-pid, &fg_status, WUNTRACED) < 0) {
                perror("waitpid");
            }

            if (WIFSTOPPED(fg_status)) {
                last_stopped_pgid = pid; // pid 也是 pgid（因為 setpgid(pid,pid)）
                printf("[stopped] pgid=%d\n", pid);
            }

            // restore terminal to shell pgid
            if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0) {
                perror("tcsetpgrp to shell");
            }
        } else {
            printf("[bg] pid=%d\n", pid);
        }

    }

    return 0;
}
