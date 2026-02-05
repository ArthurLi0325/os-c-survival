// day14/cow_mmap_private.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

static void dump_smaps_for_addr(pid_t pid, void *addr) {
    // 用 Python 讀 /proc/<pid>/smaps，找出「包含 addr 的那個 VMA」
    // 並印出你關心的欄位：Rss/Pss/Shared*/Private*
    //
    // 重點：這份版本完全不在 Python 字串裡用 % 格式化，避免被 snprintf 誤判。
    char cmd[1600];
    unsigned long a = (unsigned long)addr;

    snprintf(cmd, sizeof(cmd),
        "python3 - <<'PY'\n"
        "import re\n"
        "pid=%d\n"
        "addr=%lu\n"
        "path=f'/proc/{pid}/smaps'\n"
        "with open(path,'r') as f:\n"
        "    lines=f.readlines()\n"
        "\n"
        "def parse_range(h):\n"
        "    m=re.match(r'([0-9a-f]+)-([0-9a-f]+)\\s', h)\n"
        "    if not m:\n"
        "        return None\n"
        "    return int(m.group(1),16), int(m.group(2),16)\n"
        "\n"
        "keys=('Rss:','Pss:','Shared_Clean:','Shared_Dirty:','Private_Clean:','Private_Dirty:')\n"
        "\n"
        "i=0\n"
        "while i < len(lines):\n"
        "    r=parse_range(lines[i])\n"
        "    if r:\n"
        "        lo,hi=r\n"
        "        if lo <= addr < hi:\n"
        "            print(f'--- smaps for VMA containing addr {addr:#x} (pid={pid}) ---')\n"
        "            print(lines[i].rstrip())\n"
        "            j=i+1\n"
        "            while j < len(lines) and lines[j].strip() != '':\n"
        "                s=lines[j].strip()\n"
        "                for k in keys:\n"
        "                    if s.startswith(k):\n"
        "                        print(s)\n"
        "                        break\n"
        "                j += 1\n"
        "            break\n"
        "    i += 1\n"
        "PY",
        (int)pid, a
    );

    fflush(stdout);
    system(cmd);
}

int main(void) {
    size_t size = 4096; // 1 page
    int *p = mmap(NULL, size, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    *p = 123;
    printf("[before fork] pid=%d, p=%p, *p=%d (MAP_PRIVATE|ANON)\n",
           getpid(), (void*)p, *p);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        munmap(p, size);
        return 1;
    }

    if (pid == 0) {
        // child
        printf("[child] pid=%d, p=%p, *p(before)=%d\n", getpid(), (void*)p, *p);

        // 觸發 COW：對 MAP_PRIVATE mapping 寫入
        *p = 999;
        printf("[child] pid=%d, wrote *p=999\n", getpid());

        // 印 child 的 smaps（包含 p 的那段 VMA）
        dump_smaps_for_addr(getpid(), p);

        printf("[child] press ENTER to exit...\n");
        getchar();
        _exit(0);
    }

    // parent
    printf("[parent] pid=%d, child=%d, p=%p, *p(initial)=%d\n",
           getpid(), pid, (void*)p, *p);

    // 給 child 時間寫入並停住
    sleep(1);

    // 在 MAP_PRIVATE 下：child 寫成 999，parent 通常仍看到 123
    printf("[parent] pid=%d, now sees *p=%d (should stay 123)\n", getpid(), *p);

    // 印 parent / child 的 smaps（都針對包含 p 的那段 VMA）
    dump_smaps_for_addr(getpid(), p);
    dump_smaps_for_addr(pid, p);

    printf("[parent] press ENTER to wait child...\n");
    getchar();

    int st = 0;
    waitpid(pid, &st, 0);

    munmap(p, size);
    return 0;
}
