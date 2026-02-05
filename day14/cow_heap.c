// day14/cow_heap.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

static void dump_maps_line(const char *tag) {
    // 只印出 heap 那一行，讓你專注看重點
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "echo '--- %s (pid=%d) ---'; cat /proc/%d/maps | grep '\\[heap\\]'", 
             tag, getpid(), getpid());
    fflush(stdout);
    system(cmd);
}

int main(void) {
    // 1) 在 heap 上配置一個 int
    int *p = malloc(sizeof(int));
    if (!p) {
        perror("malloc");
        return 1;
    }
    *p = 100;

    printf("[before fork] pid=%d, p=%p, *p=%d\n", getpid(), (void*)p, *p);
    dump_maps_line("before fork");

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // child
        printf("[child] pid=%d, p=%p, *p(before)=%d\n", getpid(), (void*)p, *p);
        dump_maps_line("child after fork");

        // 2) 子行程寫入：這一筆寫入就是 COW 的觸發點
        *p = 999;

        printf("[child] pid=%d, p=%p, *p(after write)=%d\n", getpid(), (void*)p, *p);

        // 讓子行程先活著一段時間，方便你去觀察 /proc
        printf("[child] sleep 3...\n");
        sleep(90);
        _exit(0);
    }

    // parent
    printf("[parent] pid=%d, child=%d, p=%p, *p(before wait)=%d\n",
           getpid(), pid, (void*)p, *p);
    dump_maps_line("parent after fork");

    // 3) 故意等一下：讓你有時間去看 child 的 maps
    printf("[parent] sleep 1 (you can inspect /proc/%d)...\n", pid);
    sleep(1);

    // 4) 等子行程結束後，再檢查父行程看到的值
    int status = 0;
    waitpid(pid, &status, 0);

    printf("[parent] after wait, p=%p, *p=%d\n", (void*)p, *p);
    free(p);
    return 0;
}
