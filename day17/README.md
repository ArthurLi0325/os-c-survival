# Day17: Job Control Basics (Process Group & tcsetpgrp)

## 學習目標

本日重點在理解：

-   Process Group (PGID)
-   Session (SID)
-   Foreground Process Group
-   Ctrl+C (SIGINT) 的真正傳遞機制
-   setpgid() 與 tcsetpgrp() 在 shell 中的角色

核心觀念：

Ctrl+C 並不是送給某一個 PID，而是送給 Terminal 的 Foreground Process
Group。

------------------------------------------------------------------------

## 一、Process Group 與 Session 基礎

可以使用：

``` bash
ps -o pid,ppid,pgid,sid,tty,stat,cmd
```

重要欄位：

-   PID : Process ID
-   PPID : Parent Process ID
-   PGID : Process Group ID
-   SID : Session ID
-   STAT 中的 '+' 代表該 process 屬於 foreground process group

------------------------------------------------------------------------

## 二、實驗一：觀察單一程式的 PGID

程式印出：

``` c
printf("PID=%d, PPID=%d, PGID=%d, SID=%d\n",
       getpid(),
       getppid(),
       getpgid(0),
       getsid(0));
```

觀察結果：

-   PGID = PID → 代表該程式是 group leader
-   SID = shell 的 PID → 仍屬於同一個 session
-   STAT 為 S+ → 在 foreground process group

------------------------------------------------------------------------

## 三、實驗二：fork 後的行為

未呼叫 setpgid() 時：

-   parent 與 child 在同一個 PGID
-   Ctrl+C 會同時殺掉 parent 與 child

原因：

Terminal 將 SIGINT 傳送給 foreground PGID， 而 parent 與 child
屬於同一個 group。

------------------------------------------------------------------------

## 四、實驗三：child 成為獨立 PGID

在 child 中呼叫：

``` c
setpgid(0, 0);
```

結果：

-   parent PGID = 自己 PID
-   child PGID = 自己 PID
-   Ctrl+C 只殺掉 parent
-   child 存活並成為 orphan

驗證：

``` bash
ps -p <child_pid>
```

------------------------------------------------------------------------

## 五、核心實驗：tcsetpgrp()

Shell 進行 Job Control 的核心流程：

1.  fork
2.  setpgid()
3.  tcsetpgrp() 將 terminal 前景交給該 PGID
4.  waitpid()
5.  將 terminal 前景還給 shell

關鍵程式碼：

``` c
tcsetpgrp(STDIN_FILENO, child_pgid);
```

實驗結果：

-   將 terminal foreground 切給 child
-   按 Ctrl+C
-   只有 child 收到 SIGINT (signal 2)
-   parent 存活並重新取得 terminal 控制權

------------------------------------------------------------------------

## 六、今日總結

1.  Signal 是送給 Process Group，而不是單一 PID
2.  Terminal 只會將 Ctrl+C 傳給 Foreground Process Group
3.  Shell 透過 setpgid() + tcsetpgrp() 實現 Job Control
4.  Pipeline 能整組被殺，是因為它們共享同一個 PGID

------------------------------------------------------------------------

## 編譯旗標說明

本專案使用：

``` bash
gcc -O0 -g -Wall -Wextra
```

-   -O0 : 關閉最佳化，避免 debug 時程式被重排
-   -g : 產生除錯資訊，供 gdb 使用
-   -Wall : 開啟常見警告
-   -Wextra : 開啟額外警告

------------------------------------------------------------------------

## 延伸思考

-   為何 background job 不會被 Ctrl+C 影響？
-   為何 daemon 需要 setsid()？
-   Shell 如何管理多個 pipeline？
