# Day20：Process Group、Session 與 Controlling Terminal 實驗

## 今日目標

本日實作 `tty_session_lab.c`，深入理解：

-   Session（會話）
-   Process Group（行程群組）
-   Controlling Terminal（控制終端）
-   前景群組（Foreground Process Group）
-   Ctrl+C / Ctrl+Z 的訊號傳遞機制

本日實驗核心：

> 終端機只會把 Ctrl+C / Ctrl+Z 傳送給「前景 process group」，而不是單一
> process。

------------------------------------------------------------------------

## 一、Session / Process Group 結構

Unix 行程結構可簡化為：

Session └── Process Group └── Process

實驗中觀察到：

-   `sid`（Session ID）在 parent 與 child 中相同
-   `pgid`（Process Group ID）在 child 透過 `setpgid(0,0)` 後變成自己的
    pid
-   `tcgetpgrp(/dev/tty)` 會顯示目前 terminal 的前景群組

重要結論：

-   Session 不變
-   前景 Process Group 可以改變
-   訊號永遠送給前景 Process Group

------------------------------------------------------------------------

## 二、實驗流程說明

### 1. parent 啟動

-   印出 pid / pgid / sid
-   紀錄原本 terminal 的前景 pgid（shell_fg）

### 2. fork child

child 進行：

-   `setpgid(0,0)` 讓自己成為新的 Process Group leader
-   恢復 SIGINT / SIGTSTP 預設行為
-   進入 tick 迴圈

parent 進行：

-   `tcsetpgrp()` 把 terminal 前景交給 child 的 pgid
-   `waitpid(..., WUNTRACED)` 等待 child stop 或 exit

------------------------------------------------------------------------

## 三、Ctrl+Z 發生的事情

當 child 為前景群組時按 Ctrl+Z：

1.  kernel 對前景 PG 送 SIGTSTP
2.  child 進入 stopped 狀態
3.  parent 的 `waitpid(..., WUNTRACED)` 返回
4.  parent 立刻：
    -   `tcsetpgrp(shell_fg)` 把 terminal 拿回來
    -   `kill(-pgid, SIGCONT)`（模擬 fg）
    -   再次 `tcsetpgrp(child_pgid)`

重點：

> shell 必須在 STOP 後搶回 terminal，否則自己會變成背景群組。

------------------------------------------------------------------------

## 四、Ctrl+C 發生的事情

當 child 為前景群組時按 Ctrl+C：

1.  kernel 對前景 PG 送 SIGINT
2.  child 被 signal 2 終止
3.  parent 透過 waitpid 得知 WIFSIGNALED
4.  parent 把 terminal 還給 shell

------------------------------------------------------------------------

## 五、關鍵觀察

### 1. Session 不會改變

child 的 sid 與 parent 相同，表示仍在同一個 session。

### 2. 前景 PGID 會改變

`tcgetpgrp()` 的值會隨 `tcsetpgrp()` 改變。

### 3. 訊號是送給「Process Group」

-   Ctrl+Z → SIGTSTP → 前景 PG
-   Ctrl+C → SIGINT → 前景 PG

------------------------------------------------------------------------

## 六、常見錯誤：tcgetpgrp(0) 出現 Errno 25

若透過 heredoc 執行 Python：

stdin 不是 tty，而是 pipe。

因此：

`tcgetpgrp(0)` 會出現 Inappropriate ioctl for device。

正確作法：

開啟 `/dev/tty` 再呼叫 `tcgetpgrp()`。

------------------------------------------------------------------------

## 七、今日結論

1.  Terminal 控制的是「前景 Process Group」
2.  shell 的責任：
    -   建立新 Process Group
    -   切換前景群組
    -   等待 STOP / EXIT
    -   正確還原 terminal
3.  Job control 的核心是：
    -   setpgid
    -   tcsetpgrp
    -   waitpid(WUNTRACED)
    -   SIGCONT

------------------------------------------------------------------------

## 八、Day20 核心理解

真正的 job control 流程：

STOP → shell 搶回 terminal → 決定 bg 或 fg → SIGCONT → 再次交出 terminal

這正是 bash 內部實作的基本模型。
