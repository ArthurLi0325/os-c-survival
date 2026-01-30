# Day11：I/O 重導向與 Pipe（Shell 的核心機制）

## 今日目標

Day11 的目標是用 **純 C + POSIX system calls** 重現 shell 中最核心的功能：

- `>`：stdout 重導向
- `<`：stdin 重導向
- `|`：多段管線（pipeline）

完成後，我應該能清楚回答：

- 為什麼 shell 可以做到 `ls | grep c | wc -l`
- 為什麼 `pipe` 一定要 `close` 多餘的 fd
- EOF 到底是怎麼出現的

---

## 一、stdout 重導向（`>`）

### 概念

- `stdout` 本質上只是 **fd = 1**
- `printf` / `puts` 並不知道「螢幕」的存在，只是寫 fd=1
- 只要把 fd=1 改接到檔案，所有輸出就會進檔案

### 核心程式（摘要）

```c
int fd = open("out.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
dup2(fd, STDOUT_FILENO);
close(fd);

printf("hello\n");
```

### 重點結論

- `open()` 回傳值不可假設（通常是最小可用 fd）
- 永遠用 `dup2` 明確指定要接到哪個 fd

---

## 二、stdin 重導向（`<`）

### 概念

- `stdin` 只是 **fd = 0**
- `scanf / fgets / getchar` 都只是從 fd=0 讀資料
- `dup2(fd, 0)` 可以把鍵盤換成檔案

### 核心程式（摘要）

```c
int fd = open("input.txt", O_RDONLY);
dup2(fd, STDIN_FILENO);
close(fd);

while (fgets(buf, sizeof(buf), stdin)) {
    printf("read: %s", buf);
}
```

### 驗證

改用 `read(0, ...)` 一樣可以讀到檔案內容，證明不是 stdio 的魔法。

---

## 三、兩段管線（`cmd1 | cmd2`）

### 例子

```bash
ls day11 | wc -l
```

### Pipe 規則

- `pipe(p)` 產生兩個 fd
  - `p[0]`：read end
  - `p[1]`：write end

### 資料流

- `ls`：stdout → `p[1]`
- `wc`：stdin ← `p[0]`

### 關鍵程式結構

```c
pipe(p);

fork(); // ls
dup2(p[1], 1);

fork(); // wc
dup2(p[0], 0);
```

### 極重要結論

> **只要還有任何 process 持有 pipe 的寫端，read 端就永遠收不到 EOF。**

因此：

- parent 一定要關掉 pipe fd
- child 也必須關掉「不是自己用的 fd」

---

## 四、三段管線（`cmd1 | cmd2 | cmd3`）

### 例子

```bash
ls day11 | grep c | wc -l
```

### 規則

- 3 個 command → 2 條 pipe → 3 個 child process

### Pipe 配置

- pipe1：ls → grep
- pipe2：grep → wc

### 每個 process 的角色

- `ls`：stdout → pipe1 write
- `grep`：stdin ← pipe1 read，stdout → pipe2 write
- `wc`：stdin ← pipe2 read

### 為什麼會卡？（實驗）

若 **任何一個 process**（尤其是 parent 或 grep）沒有關掉 `pipe2` 的寫端：

- `wc` 永遠等不到 EOF
- 整個程式卡住

### 標準結論

> Pipe 是否結束，**不看誰還在跑**，而是看「寫端 fd 的 reference count 是否歸零」。

---

## 五、今日總結

Day11 我實作並驗證了：

- stdin / stdout 都只是可重導向的 fd
- `dup2` 是 I/O 重導向的核心
- `pipe` 本質是 kernel buffer + fd reference count
- 忘記 `close` pipe fd 是 pipeline 卡住的唯一常見原因

這一天完成後，shell 的 `> < |` 不再是黑魔法，而是可以完全自己重現的機制。

---