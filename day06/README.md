# Day06：Process 啟動流程與真正的程式入口 `_start`

## 本日目標
理解 **Linux 程式並非從 `main()` 開始執行**，而是從 ELF entry point `_start` 開始，並實際用 `gdb` 驗證：

- kernel 如何建立 initial stack（`argc / argv / envp`）
- `_start` 如何依 ABI 整理參數
- `_start` 如何呼叫 `__libc_start_main`
- `__libc_start_main` 如何呼叫 `main`

---

## 本日檔案
```text
day06/
├── argv_layout.c   // 印出 argc / argv / envp
└── argv_layout     // 編譯後執行檔
```

---

## 1️. argv / envp 來自哪裡？

### 程式碼：`argv_layout.c`
```c
#include <stdio.h>

int main(int argc, char *argv[], char *envp[]) {
    printf("argc = %d\n", argc);

    for (int i = 0; argv[i] != NULL; i++) {
        printf("argv[%d] = %p -> %s\n", i, argv[i], argv[i]);
    }

    printf("\n--- envp ---\n");
    for (int i = 0; envp[i] != NULL && i < 5; i++) {
        printf("envp[%d] = %p -> %s\n", i, envp[i], envp[i]);
    }
    return 0;
}
```

### 編譯
```bash
gcc -O0 -g argv_layout.c -o argv_layout
```
> - `-O0`：關閉最佳化，確保反組譯與 gdb 行為直觀  
> - `-g`：加入除錯符號，讓 gdb 可解析 symbol

### 執行
```bash
./argv_layout hello world
```

---

## 2️. Process 啟動時的 stack（main 之前）

```bash
gdb ./argv_layout
(gdb) starti
```

觀察 stack：
```gdb
x/32gx $rsp
```

邏輯結構：
```text
[rsp]     = argc
[rsp+8]   = argv[0]
[rsp+16]  = argv[1]
...
[rsp+...] = NULL
[rsp+...] = envp[0]
...
```

---

## 3️. 為什麼 starti 會停在 ld-linux？

因為程式是 **動態連結 ELF**：

```text
kernel
  ↓
ld-linux (_start)
  ↓
載入 libc、做 relocation
  ↓
跳到「程式自己的 _start」
```

---

## 4️. ELF Entry Point

```bash
readelf -h ./argv_layout | grep 'Entry point'
```

輸出：
```text
Entry point address: 0x1080
```

---

## 5️. 停在「程式自己的 _start」

```gdb
(gdb) break *_start
(gdb) run
```

---

## 6. `_start` 反組譯（關鍵）

```gdb
x/30i $pc
```

關鍵指令：
```asm
pop    %rsi      # argc
mov    %rsp,%rdx # argv
lea    ...,%rdi  # &main
call   *...      # __libc_start_main
```

---

## 7. ABI 參數對應（System V AMD64）

| 暫存器 | 內容 |
|------|------|
| rdi | main |
| rsi | argc |
| rdx | argv |
| rcx | init |
| r8  | fini |
| r9  | rtld_fini |

---

## 8. 驗證 __libc_start_main

```gdb
break __libc_start_main
set args hello world
run
```

```text
main=0x555555555169
argc=3
argv=0x7fffffffe008
```

---

## 結論

- C 程式不是從 `main()` 開始
- kernel 建立 stack
- `_start` 呼叫 `__libc_start_main`
- libc 再呼叫 `main()`