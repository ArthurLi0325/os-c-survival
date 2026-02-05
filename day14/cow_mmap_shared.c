// day14/cow_mmap_shared.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

static void dump_smaps_one(pid_t pid, void *addr) {
    // 只印「包含 addr 的那個 VMA」的 header + 6 個欄位
    char cmd[1800];
    unsigned long a = (unsigned long)addr;

    snprintf(cmd, sizeof(cmd),
        "python3 - <<'PY'\n"
        "import re\n"
        "pid=%d\n"
        "addr=%lu\n"
        "keys=('Rss:','Pss:','Shared_Clean:','Shared_Dirty:','Private_Clean:','Private_Dirty:')\n"
        "lines=open(f'/proc/{pid}/smaps','r').read().splitlines()\n"
        "def rng(h):\n"
        "    m=re.match(r'([0-9a-f]+)-([0-9a-f]+)\\s', h)\n"
        "    if not m: return None\n"
        "    return int(m.group(1),16), int(m.group(2),16)\n"
        "for i,h in enumerate(lines):\n"
        "    r=rng(h)\n"
        "    if not r: continue\n"
        "    lo,hi=r\n"
        "    if lo<=addr<hi:\n"
        "        print(f'--- smaps (pid={pid}) for addr {addr:#x} ---')\n"
        "        print(h)\n"
        "        picked=0\n"
        "        j=i+1\n"
        "        while j<len(lines) and lines[j].strip()!='' and picked<len(keys):\n"
        "            s=lines[j].strip()\n"
        "            for k in keys:\n"
        "                if s.startswith(k):\n"
        "                    print(s)\n"
        "                    picked+=1\n"
        "                    break\n"
        "            j+=1\n"
        "        break\n"
        "PY",
        (int)pid, a
    );

    fflush(stdout);
    system(cmd);
}

int main(void) {
    size_t size = 4096; // 1 page
    int *p = mmap(NULL, size, PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    *p = 123;
    printf("[before fork] pid=%d, p=%p, *p=%d (MAP_SHARED|ANON)\n",
           getpid(), (void*)p, *p);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        munmap(p, size);
        return 1;
    }

    if (pid == 0) {
        printf("[child] pid=%d, p=%p, *p(before)=%d\n", getpid(), (void*)p, *p);

        *p = 999;
        printf("[child] pid=%d, wrote *p=999\n", getpid());

        // child 的 smaps
        dump_smaps_one(getpid(), p);

        printf("[child] press ENTER to exit...\n");
        getchar();
        _exit(0);
    }

    printf("[parent] pid=%d, child=%d, p=%p, *p(initial)=%d\n",
           getpid(), pid, (void*)p, *p);

    sleep(1);

    // 在 MAP_SHARED 下：child 改成 999，parent 也應該看到 999
    printf("[parent] pid=%d, now sees *p=%d (should become 999)\n", getpid(), *p);

    // parent / child smaps（都針對包含 p 的 VMA）
    dump_smaps_one(getpid(), p);
    dump_smaps_one(pid, p);

    printf("[parent] press ENTER to wait child...\n");
    getchar();

    int st = 0;
    waitpid(pid, &st, 0);

    munmap(p, size);
    return 0;
}
