# Day19 - Mini Shell 與基礎 Job Control

## 今日目標

實作一個最小可用的 Shell，並加入基礎 Job Control 功能。

透過本日練習，將前 18 天學過的：

-   fork / exec
-   waitpid
-   background process
-   zombie 回收
-   process group
-   tcsetpgrp
-   SIGINT / SIGTSTP
-   SIGCONT

整合成一個實際可運作的迷你 shell。

------------------------------------------------------------------------

## 功能列表

目前 mini_shell 支援：

1.  執行一般指令

        mini$ ls
        mini$ echo hello
        mini$ sleep 1

2.  Background 執行（使用 &）

        mini$ sleep 10 &

3.  自動回收 background child（避免 zombie）

4.  Ctrl+C 只中斷前景程序，不會殺死 shell

5.  Ctrl+Z 可暫停前景程序

6.  fg 可將最近被 Ctrl+Z 停止的程序拉回前景

------------------------------------------------------------------------

## 核心設計概念

### 1. fork + execvp

Shell 使用：

    fork()
    execvp()
    waitpid()

建立並執行子程序。

------------------------------------------------------------------------

### 2. Background Process

當偵測到最後一個參數為 `&`：

-   不呼叫 blocking wait
-   立即回到 prompt
-   使用 waitpid(..., WNOHANG) 回收已結束 child

------------------------------------------------------------------------

### 3. Zombie 回收機制

在每次 prompt 前執行：

    waitpid(-1, &status, WNOHANG)

確保已完成的 background child 不會成為 defunct。

------------------------------------------------------------------------

### 4. Process Group 與 Terminal 控制

每個 child：

    setpgid(0, 0);

使其成為新的 process group leader。

前景工作時：

    tcsetpgrp(STDIN_FILENO, child_pgid);

工作完成或停止後：

    tcsetpgrp(STDIN_FILENO, shell_pgid);

將 terminal 控制權還給 shell。

------------------------------------------------------------------------

### 5. Signal 控制策略

Shell 本身忽略：

-   SIGINT
-   SIGTSTP
-   SIGTTIN
-   SIGTTOU

Child 在 exec 前恢復為：

    SIG_DFL

確保 Ctrl+C / Ctrl+Z 正確作用於前景工作。

------------------------------------------------------------------------

### 6. fg 實作方式

-   記錄最近一次被停止的 process group

-   使用：

        kill(-pgid, SIGCONT)

    送出繼續訊號

-   再次使用 tcsetpgrp 交出 terminal

-   waitpid 等待結束或再次停止

------------------------------------------------------------------------

## 測試範例

    mini$ sleep 30
    ^Z
    [stopped] pgid=1234
    mini$ fg
    ^C
    mini$

行為與一般 shell 一致。

------------------------------------------------------------------------

## 今日學習重點

1.  Shell 必須控制 process group
2.  Terminal 只會將 Ctrl+C / Ctrl+Z 傳給前景 group
3.  若不忽略 SIGTTOU / SIGTTIN，shell 會被 stop
4.  子程序會繼承父程序的 signal disposition
5.  Job Control 的核心是：
    -   setpgid
    -   tcsetpgrp
    -   kill(-pgid, ...)
    -   waitpid(..., WUNTRACED)

------------------------------------------------------------------------

## 延伸方向（Day20）

-   支援多個 job
-   jobs 指令
-   bg 指令
-   完整 job table
-   SIGCHLD handler
-   pipeline + job control 結合

------------------------------------------------------------------------

## 總結

Day19 將 fork/exec、signal、wait、process group、 terminal control
等概念整合為一個可運作的 mini shell。

這是系統程式設計中的重要里程碑。