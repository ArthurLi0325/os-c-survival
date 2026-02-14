# Day18 -- Job Control 與前景程序控制

## 今日目標

理解 UNIX Job Control 的核心機制，並實作一個最小可運作的互動式 job
control 程式。

今天實際完成：

-   建立獨立 process group
-   使用 tcsetpgrp() 控制 terminal 前景
-   處理 Ctrl+Z（SIGTSTP）
-   使用 waitpid(..., WUNTRACED) 偵測 stopped
-   使用 SIGCONT 讓 job 繼續執行
-   正確終止 stopped job（SIGCONT + SIGTERM）
-   使用 waitpid(-pgid, ...) 回收整個 process group

------------------------------------------------------------------------

## 核心觀念

### 1. Terminal 只會對「前景 process group」送信號

當你按：

-   Ctrl+C → 送 SIGINT
-   Ctrl+Z → 送 SIGTSTP

terminal driver 會把信號送給：

> foreground process group

因此，若 child 要能被 Ctrl+Z 停止，必須：

    setpgid(0, 0);
    tcsetpgrp(STDIN_FILENO, child_pgid);

------------------------------------------------------------------------

### 2. 為什麼 parent 要忽略 SIGTTOU / SIGTTIN？

當 background process 嘗試操作 terminal（例如 tcsetpgrp）， kernel
會送：

-   SIGTTOU
-   SIGTTIN

shell 會忽略這些信號：

    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

否則 parent 會被停住。

------------------------------------------------------------------------

### 3. 為什麼 child 要恢復預設信號？

child 必須正常接收 Ctrl+C / Ctrl+Z：

    signal(SIGTSTP, SIG_DFL);
    signal(SIGINT,  SIG_DFL);

否則 Ctrl+Z 不會生效。

------------------------------------------------------------------------

### 4. 為什麼 waitpid 需要 WUNTRACED？

預設 waitpid 只會回傳 exit 或被 signal kill。

若要偵測「被 Ctrl+Z 停止」，必須使用：

    waitpid(pid, &status, WUNTRACED);

否則 parent 永遠不會知道 child 被 stop。

------------------------------------------------------------------------

### 5. 為什麼 kill stopped job 需要 SIGCONT？

Stopped process 收到 SIGTERM 時可能不會立刻退出， 必須先讓它繼續執行：

1.  SIGCONT
2.  SIGTERM
3.  waitpid 回收

```{=html}
<!-- -->
```
    kill(-pgid, SIGCONT);
    kill(-pgid, SIGTERM);
    waitpid(-pgid, ...);

------------------------------------------------------------------------

## 程式流程

1.  fork child

2.  child 建立新 process group

3.  parent 把 terminal 前景交給 child

4.  child 執行並等待 Ctrl+Z

5.  parent 使用 waitpid(WUNTRACED) 偵測 stopped

6.  terminal 還給 parent

7.  進入互動模式：

    -   fg → SIGCONT + tcsetpgrp
    -   kill → SIGCONT + SIGTERM + waitpid
    -   exit → 離開

------------------------------------------------------------------------

## 編譯

    gcc -O0 -g -Wall -Wextra -o mini_job_control mini_job_control.c

------------------------------------------------------------------------

## 執行

    ./mini_job_control

操作流程：

1.  程式開始印 tick
2.  按 Ctrl+Z
3.  輸入 fg
4.  再按 Ctrl+Z
5.  輸入 kill 結束

------------------------------------------------------------------------

## 今日學習成果
# Day18 -- Job Control 與前景程序控制

## 今日目標

理解 UNIX Job Control 的核心機制，並實作一個最小可運作的互動式 job
control 程式。

今天實際完成：

-   建立獨立 process group
-   使用 tcsetpgrp() 控制 terminal 前景
-   處理 Ctrl+Z（SIGTSTP）
-   使用 waitpid(..., WUNTRACED) 偵測 stopped
-   使用 SIGCONT 讓 job 繼續執行
-   正確終止 stopped job（SIGCONT + SIGTERM）
-   使用 waitpid(-pgid, ...) 回收整個 process group

------------------------------------------------------------------------

## 核心觀念

### 1. Terminal 只會對「前景 process group」送信號

當你按：

-   Ctrl+C → 送 SIGINT
-   Ctrl+Z → 送 SIGTSTP

terminal driver 會把信號送給：

> foreground process group

因此，若 child 要能被 Ctrl+Z 停止，必須：

    setpgid(0, 0);
    tcsetpgrp(STDIN_FILENO, child_pgid);

------------------------------------------------------------------------

### 2. 為什麼 parent 要忽略 SIGTTOU / SIGTTIN？

當 background process 嘗試操作 terminal（例如 tcsetpgrp）， kernel
會送：

-   SIGTTOU
-   SIGTTIN

shell 會忽略這些信號：

    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

否則 parent 會被停住。

------------------------------------------------------------------------

### 3. 為什麼 child 要恢復預設信號？

child 必須正常接收 Ctrl+C / Ctrl+Z：

    signal(SIGTSTP, SIG_DFL);
    signal(SIGINT,  SIG_DFL);

否則 Ctrl+Z 不會生效。

------------------------------------------------------------------------

### 4. 為什麼 waitpid 需要 WUNTRACED？

預設 waitpid 只會回傳 exit 或被 signal kill。

若要偵測「被 Ctrl+Z 停止」，必須使用：

    waitpid(pid, &status, WUNTRACED);

否則 parent 永遠不會知道 child 被 stop。

------------------------------------------------------------------------

### 5. 為什麼 kill stopped job 需要 SIGCONT？

Stopped process 收到 SIGTERM 時可能不會立刻退出， 必須先讓它繼續執行：

1.  SIGCONT
2.  SIGTERM
3.  waitpid 回收

```{=html}
<!-- -->
```
    kill(-pgid, SIGCONT);
    kill(-pgid, SIGTERM);
    waitpid(-pgid, ...);

------------------------------------------------------------------------

## 程式流程

1.  fork child

2.  child 建立新 process group

3.  parent 把 terminal 前景交給 child

4.  child 執行並等待 Ctrl+Z

5.  parent 使用 waitpid(WUNTRACED) 偵測 stopped

6.  terminal 還給 parent

7.  進入互動模式：

    -   fg → SIGCONT + tcsetpgrp
    -   kill → SIGCONT + SIGTERM + waitpid
    -   exit → 離開

------------------------------------------------------------------------

## 編譯

    gcc -O0 -g -Wall -Wextra -o mini_job_control mini_job_control.c

------------------------------------------------------------------------

## 執行

    ./mini_job_control

操作流程：

1.  程式開始印 tick
2.  按 Ctrl+Z
3.  輸入 fg
4.  再按 Ctrl+Z
5.  輸入 kill 結束

------------------------------------------------------------------------

## 今日學習成果

-   Session / Process Group / Terminal 三層關係
-   為什麼 job control 必須使用 process group
-   為什麼 shell 要忽略 SIGTTOU
-   為什麼 child 必須恢復預設信號
-   為什麼 waitpid 需要 WUNTRACED
-   如何安全終止 stopped job

這是 shell 設計的核心機制。