# Day10：Pipe、dup2 與 I/O Redirection

## 今日目標

今天的重點不是「寫出一個 shell」，而是**理解 shell pipeline 背後真正發生的 OS 行為**。

在完成 Day10 後，我應該能夠清楚回答：

- 為什麼 `ls | wc -l` 需要 fork 兩次
- 為什麼 `dup2` 一定要在 `exec` 之前
- 為什麼忘記 `close(pipe)` 會造成程式卡死
- 為什麼程式本身（ls / wc）不知道自己被重導向

---

## 核心概念總覽

Day10 的所有程式，其實都圍繞在以下四個系統概念：

1. pipe 是 kernel object，不是語言語法
2. fork 會完整複製 fd table
3. dup2 只是「換線」，不是資料搬移
4. exec 不會重設 fd table

---

## 一、pipe 是什麼

`pipe(fd)` 會在 kernel 中建立一個管道物件，並回傳兩個檔案描述符：

- `fd[0]`：讀端（read end）
- `fd[1]`：寫端（write end）

重要性質：

- pipe 是單向的
- read 在沒有資料時會 block
- 只有當「所有寫端都被關閉」時，讀端才會看到 EOF

這也是為什麼 **忘記關 pipe 幾乎一定會導致 deadlock**。

---

## 二、fork 與 fd table 的複製

呼叫 `fork()` 時，child process 會：

- 複製 parent 的虛擬記憶體
- **完整複製 fd table**

這代表：

- parent 與 child 一開始握有「相同的 pipe 兩端」
- 如果不主動 `close` 不需要的 fd，EOF 永遠不會出現

---

## 三、dup2 的真正意義

### 函式原型

```c
int dup2(int oldfd, int newfd);
```

### 精確語意

> `dup2(oldfd, newfd)` 會讓 **newfd 指向 oldfd 所指向的 kernel object**。

重點：

- 被改的是 `newfd`
- `oldfd` 完全不動
- 不是資料 copy，只是 fd 指向的對象改變

### 例子

```c
dup2(fd[1], STDOUT_FILENO);
```

意思是：

- stdout（fd=1）不再指向 terminal
- stdout 改指向 pipe 的寫端

此後任何：

```c
printf(...)
write(1, ...)
```

實際上都是寫進 pipe。

---

## 四、為什麼 exec 前一定要 dup2

關鍵事實：

> **exec 不會重設 fd table**

流程如下：

1. fork 建立 child
2. 在 child 中使用 dup2 重新接好 stdin / stdout
3. 呼叫 exec 換成新程式

結果是：

- 程式碼被換掉（例如 ls / wc）
- fd table 完整保留

因此 ls / wc 完全不知道：

- 自己的 stdin / stdout 已經被重導向

---

## 五、實作一：pipe_fork.c

此程式驗證：

- pipe 的 blocking 行為
- EOF 與 close 的關係
- parent / child 如何透過 pipe 傳資料

關鍵觀察：

- child 若未關閉寫端，read 永遠等不到 EOF
- pipe 的生命週期與 fd 是否被關閉高度相關

---

## 六、實作二：pipe_exec.c（ls | wc -l）

此程式完整重現 shell pipeline：

```bash
ls | wc -l
```

實作策略：

- 建立一個 pipe
- fork 第一個 child，dup2 pipe 寫端到 stdout，exec ls
- fork 第二個 child，dup2 pipe 讀端到 stdin，exec wc -l
- parent 關閉所有 pipe 並 wait

此設計直接說明：

> pipeline = fork × N + pipe × (N - 1)

---

## 七、為什麼 parent 一定要關 pipe

EOF 出現的條件是：

> **所有寫端 fd 都被關閉**

如果 parent 還握著寫端：

- wc 永遠看不到 EOF
- wc 會一直 block
- 整條 pipeline 卡死

因此：

- parent 必須在 fork 後立刻關掉 pipe

---

## 今日總結

Day10 真正理解的不是 API，而是這件事：

> **shell 的本質，是在 exec 前幫你把 fd 接好**。

pipe 提供資料通道，
dup2 負責接線，
exec 只是換程式碼。

當這三件事連在一起，pipeline 就自然出現了。

---

