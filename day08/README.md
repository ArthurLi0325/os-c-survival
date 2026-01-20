# Day08 — Signals、SIGCHLD、Zombie 與 kill

## 本日目標
本日重點在於理解 **Signal（訊號）** 在 Linux 中的角色，以及它如何影響正在執行的 process。
透過實作與觀察，我們將釐清以下問題：

- Signal 是什麼？何時、如何被送出？
- 為什麼 system call 會被 signal 中斷（EINTR）？
- 為什麼 signal handler 不能隨便寫？
- child 結束後為什麼會變成 zombie？
- kill 指令真的「一定會殺掉」程式嗎？

---

## 1. SIGINT（Ctrl+C）與 EINTR

### signal_basic.c
- 使用 `sigaction()` 註冊 SIGINT handler。
- 攔截 Ctrl+C，使程式不會直接結束。
- 移除 `SA_RESTART` 後，觀察 blocking system call 被中斷的行為。

關鍵觀察：
- Ctrl+C 會送出 SIGINT，而不是鍵盤直接終止程式。
- 若沒有 `SA_RESTART`，`sleep()` 可能會提早返回。
- `errno` 會被設為 `EINTR`（Interrupted system call）。

範例輸出概念：
```
^Csleep interrupted! left = 1 (errno=4: Interrupted system call)
[main] observed SIGINT flag
```

補充說明：
- `^C` 是 terminal driver 的回顯，不是程式輸出，因此不一定會自動換行。

---

## 2. 正確的 signal handler 寫法（flag 模式）

### 為什麼 handler 不能使用 printf？
Signal handler 可能在任何時間點插入執行，包含程式正在執行 libc 內部（例如 stdio 或 malloc）。
若在 handler 中呼叫非 async-signal-safe 的函式，可能導致：

- deadlock
- 資料結構損壞
- undefined behavior

### 正確做法
- handler 中只設定一個 `volatile sig_atomic_t` 旗標。
- 在 main loop 中偵測該旗標，再進行輸出或其他邏輯。

這是系統程式設計中最常見、也最安全的 signal 處理模式。

---

## 3. SIGCHLD 與 child 回收

### sigchld.c
- parent fork 出 child。
- child sleep 後呼叫 `_exit(42)`。
- parent 註冊 SIGCHLD handler。
- 在主流程中使用：

```c
waitpid(-1, &status, WNOHANG)
```

關鍵觀察：
- child 結束時，kernel 會送 SIGCHLD 給 parent。
- parent 若有正確 wait，child 不會成為 zombie。

範例輸出：
```
[parent] reaped child 15369, exit=42
[parent] no more children
```

---

## 4. Zombie process 的產生與觀察

### zombie_demo.c
- parent 不呼叫 `wait()`。
- child 立刻結束。
- parent 繼續存活。

使用 `ps` 觀察：
```
PID    PPID STAT CMD
17039     304 S+   ./day08/zombie_demo
17040   17039 Z+   [zombie_demo] <defunct>
```

說明：
- Zombie 不是「還在跑的程式」。
- 它是已結束，但 exit status 尚未被 parent 回收的 process。

---

## 5. Zombie 的回收

### zombie_reap.c
- parent 先產生 zombie。
- 等待使用者按 Enter。
- 呼叫 `waitpid()` 回收 child。

觀察結果：
- 回收前：child 顯示為 `Z+ <defunct>`。
- 回收後：child 從 `ps` 輸出中完全消失。

這證明 zombie 只是暫存狀態，並非真正佔用 CPU。

---

## 6. SIGTERM vs SIGKILL（kill 的真正意義）

### term_vs_kill.c
- 註冊 SIGTERM handler，模擬 graceful shutdown。
- 比較 SIGTERM 與 SIGKILL 的行為差異。

實驗結果：

- SIGTERM：
  - 可被攔截。
  - 程式有機會做 cleanup。
  - 最後自行 `exit()`。

- SIGKILL：
  - 無法被攔截、忽略或處理。
  - kernel 直接終止 process。
  - 程式無法執行任何 cleanup。

結論：
- `kill` 這個名字具有誤導性。
- 它實際上是「送出 signal」，而非保證終止程式。

---

## 編譯方式

```bash
gcc -O0 -g -Wall -Wextra -o day08/signal_basic day08/signal_basic.c
gcc -O0 -g -Wall -Wextra -o day08/sigchld day08/sigchld.c
gcc -O0 -g -Wall -Wextra -o day08/zombie_demo day08/zombie_demo.c
gcc -O0 -g -Wall -Wextra -o day08/zombie_reap day08/zombie_reap.c
gcc -O0 -g -Wall -Wextra -o day08/term_vs_kill day08/term_vs_kill.c
```

編譯選項說明：
- `-O0`：關閉最佳化，方便除錯與對應原始碼。
- `-g`：產生除錯資訊，供 gdb 使用。
- `-Wall -Wextra`：開啟額外警告，提早發現潛在問題。

---

## 本日重點總結
- Signal 是非同步事件，可能在任何時間中斷程式。
- Blocking system call 可能因 signal 提早返回（EINTR）。
- Signal handler 必須極度簡潔，避免呼叫不安全函式。
- child 結束後一定要 wait，否則會產生 zombie。
- SIGTERM 是請求，SIGKILL 是強制。