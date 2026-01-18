# Day07 – Process, fork, exec 與 gdb 觀察

專案：os-c-survival  
進度：Day07  
主題：Process、fork / exec 行為，以及使用 gdb 觀察 instruction pointer（RIP）的分裂

---

## 一、Day07 目標

Day07 的目標是從「單一程式的記憶體視角」，正式進入 **OS 對 process 的管理視角**：

- 理解 **program 與 process 的差異**
- 搞懂 `fork()` 如何建立新 process
- 理解 **Copy-on-Write (COW)** 為何讓 fork 高效
- 搞懂 `exec()` 如何在 **PID 不變的情況下替換整個程式映像**
- 使用 **gdb** 親眼觀察 fork / exec 時 instruction pointer（RIP）的行為

---

## 二、Process 與 Program 的差別

| 名稱 | 說明 |
|----|----|
| Program | 磁碟上的可執行檔（例如 fork_exec） |
| Process | 程式被 OS 載入後的執行實體 |
| PID | OS 用來識別 process 的唯一編號 |

**同一個 program 可以同時存在多個 process（PID 不同）**

---

## 三、fork()：建立新 Process

### fork 的行為

- `fork()` 會產生一個 **新的 process**
- parent 與 child：
  - 擁有 **不同 PID**
  - 擁有「看起來一樣」的虛擬位址空間
- fork 回傳值：
  - child：`fork() == 0`
  - parent：`fork() == child_pid`

---

## 四、fork + stack（fork_stack.c）

### 觀察重點

```c
int x = 10;
pid_t pid = fork();
```

執行結果顯示：

- parent 與 child 的 `&x`（虛擬位址）相同
- 但修改 `x` 不會互相影響

### 結論

- `&x` 是 **虛擬位址**
- fork 後兩個 process 各自擁有獨立的 address space
- OS 使用 **Copy-on-Write (COW)** 確保效率與隔離

---

## 五、fork + heap（fork_heap.c）

### 觀察重點

```c
int *p = malloc(sizeof(int));
*p = 5;
pid_t pid = fork();
```

- parent / child 的 `p` 數值（虛擬位址）相同
- 但 `*p` 各自修改後不互相影響

### 結論

- heap 不是立即被完整複製
- fork 時先共享頁面（read-only）
- 寫入時才複製（Copy-on-Write）

---

## 六、fork + exec（fork_exec.c）

### fork 與 exec 的分工

| 函式 | 功能 |
|----|----|
| fork | 決定「有幾個 process」 |
| exec | 決定「process 變成哪個程式」 |

### exec 的關鍵特性

- `exec()` **不會回傳（成功時）**
- PID 不變
- code / stack / heap / global data 全部被新程式取代

---

## 七、使用 gdb 觀察 fork / exec（instruction pointer）

### gdb 設定

```gdb
set follow-fork-mode child
set detach-on-fork off
set schedule-multiple on

catch fork
catch exec
break main
run
```

---

### fork：instruction pointer 分裂

- gdb 在 `__GI__Fork` 停住
- fork 完成後產生 child PID（例如 30802）
- parent / child 從 fork return 附近「同點分裂」
- 分岔原因：**fork 回傳值不同**

```text
parent: pid = 30802
child : pid = 0
```

---

### exec：instruction pointer 跳轉

在 child 端觸發 `exec("/usr/bin/ls")` 後：

- RIP 不再回到 fork_exec.c
- RIP 直接跳到 dynamic loader 的 `_start`
- `info proc` 顯示：

```text
exe = /usr/bin/ls
```

證明 **process image 已被完整替換**

---

### parent：等待 child

在 parent 端可看到：

```text
__GI___wait4(pid=30802, ...)
```

- 表示 parent 正在等待 child 結束
- `waitpid` 是 process 同步的基本機制

---

## 八、Day07 核心結論

> **OS 管理的是 process，而不是 function 或 binary。**  
> **fork 決定「有幾個 process」，exec 決定「這個 process 是誰」。**

---

## 九、今日收穫

- fork 並不是複製整個記憶體，而是靠 COW
- exec 不是建立新 process，而是「靈魂轉生」
- gdb 可以用來觀察 OS 層級的行為，而不只是 debug bug

---