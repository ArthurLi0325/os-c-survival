# Day09 – Process Synchronization（wait / waitpid）

本日重點在於理解 **parent / child process 的同步關係**，以及 Linux 為何需要 `wait / waitpid` 這組 API 來回收 child process，避免 zombie process 的產生。

---

## 今日目標

- 理解 parent 與 child 的生命週期關係
- 了解 zombie process 的成因與實際觀察方式
- 正確解析 `wait()` 所回傳的 status
- 理解 `wait()` 與 `waitpid()` 的差異
- 學會使用 `WNOHANG` 實作 non-blocking 回收
- 知道實務上如何避免 zombie process 堆積

---

## 核心觀念總覽

- child 呼叫 `exit()` 並不代表資源立即被釋放
- child 結束後，kernel 會暫時保留 exit status
- **回收 child 是 parent 的責任**
- zombie process 是「已結束但尚未被回收」的 child
- `wait` / `waitpid` 是 parent 與 kernel 的同步介面

---

## fork + wait：最小同步範例

### 行為觀察

- parent 呼叫 `wait()` 會 block
- child 結束後，parent 才會繼續執行
- 沒有 `wait()`，parent 可能先結束

### 重點結論

`wait()` 讓 parent 與 child 在「結束時刻」同步。

---

## Zombie process 的產生

### 成因

1. child 呼叫 `exit()`
2. parent 尚未呼叫 `wait()`
3. kernel 保留 child 的 exit status
4. child 狀態變為 `Z`（zombie）

### 實際觀察

使用以下指令可觀察 zombie：

```
ps -o pid,ppid,stat,cmd
```

典型輸出：

- `STAT = Z`
- `CMD` 顯示為 `<defunct>`

### 關鍵結論

zombie 不是還在執行的程式，而是 **尚未被 parent 回收的結束狀態**。

---

## wait status：child 是怎麼死的？

`wait(&status)` 會將 child 的死亡資訊編碼進 `status`。

### 正常結束（exit）

```c
WIFEXITED(status)
WEXITSTATUS(status)
```

- `WIFEXITED` 為真
- `WEXITSTATUS` 為 `exit(n)` 中的 `n`

### 被 signal 終止

```c
WIFSIGNALED(status)
WTERMSIG(status)
```

- `WIFSIGNALED` 為真
- `WTERMSIG` 為 signal 編號
- 例如 SIGKILL = 9

### 關鍵結論

child 的 exit code 或 signal **不是給 OS 用的，而是給 parent 判斷 child 結束方式**。

---

## wait vs waitpid

### wait()

- 阻塞
- 回收「任一」已結束的 child
- 無法指定 PID

### waitpid(pid, ...)

- 可指定要回收哪個 child
- `pid = -1`：任一 child
- 只等特定 pid 時，其餘 child 可能暫時成為 zombie

---

## WNOHANG：non-blocking 回收

### 行為說明

```c
waitpid(pid, &status, WNOHANG)
```

- 回傳 0：目前沒有可回收的 child
- 回傳 PID：成功回收
- parent 不會被 block

### 實務陷阱

若只對特定 PID 使用 `WNOHANG`：
- 其他已結束的 child 可能先變 zombie

---

## 實務正解：回收所有已結束 child

推薦做法：

```c
waitpid(-1, &status, WNOHANG)
```

搭配 loop：

- parent 持續做其他工作
- 每次 loop 順便回收所有已死 child
- 不會 block
- 不會產生 zombie 堆積

---

## 與 Day08（signal）的連結

- Day08：signal 決定 child「怎麼死」
- Day09：wait / waitpid 決定 parent「怎麼知道 child 怎麼死」

兩者合起來，才是完整的 process lifecycle 管理。

---

## 今日總結

- zombie 是設計結果，不是 bug
- parent 必須負責回收 child
- `wait()` 是阻塞同步
- `waitpid()` 提供控制權
- `WNOHANG` 讓 parent 可以邊做事邊回收
- 實務上應使用 `waitpid(-1, ..., WNOHANG)` 避免 zombie

---
