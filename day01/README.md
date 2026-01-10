# Day01 — C Program Execution & Memory Basics

## 目標（Goals）
Day01 的目標是建立「C 程式在 OS 底下如何被執行」的直覺，而不只是語法層面的理解。
完成本日內容後，應能理解：

- C 程式如何變成一個 process
- `main` 的 return value 如何回到 shell
- 為什麼 `printf` 有時候會「沒印出來」
- segmentation fault 的真正成因
- 如何用 gdb 把 crash 轉成可讀線索

---

## 環境（Environment）

- OS: Windows + WSL2 (Ubuntu)
- Compiler: gcc
- Debugger: gdb
- Editor: VSCode (Remote - WSL)

---

## 內容與實驗（Experiments）

### 1. Minimal C Program & Exit Status

檔案：`hello.c`

```c
int main() {
    return 0;
}

重點觀念：
main 由 OS 的 runtime 呼叫
return 0 成為 process 的 exit status
shell 可用 $? 取得上一個 process 的結束狀態
```

### 2. stdout Buffering 與 printf 行為

檔案：printf.c

```c
int main() {
    printf("Hello");
    return 0;
}

觀察：
沒有 \n 時，輸出可能不會立即顯示
在正常結束時，buffer 會被 flush
若程式因 segfault 非正常結束，buffer 可能來不及 flush
```

### 3. Segmentation Fault 的本質

```c
int main() {
    printf("Hello");
    int *p = 0;
    *p = 1;
    return 0;
}

理解重點：
p = 0 等同於 p = NULL
解參考 NULL 代表存取不可存取的 virtual memory page
fault 由 MMU 偵測，OS 發送 SIGSEGV 終止 process
shell 顯示 exit code 139 = 128 + SIGSEGV(11)
```

### 4. 使用 gdb 進行基本除錯

編譯（含 debug symbol）：
```bash
gcc -g printf.c -o printf
```
gdb 操作流程：
```gdb
run
bt
list
print p
print *p
info locals
```

- 重點收穫：
- crash 可以被精準定位到「哪一行」
- 可以即時檢查變數狀態
- segmentation fault 不再是黑盒事件

## 總結
- C 程式的執行結果由 OS 決定，而非語言本身
- segmentation fault 是記憶體保護機制的結果，不是隨機錯誤
- gdb 是系統程式設計中不可或缺的工具
- debug 的本質是「驗證假設」，而非猜測