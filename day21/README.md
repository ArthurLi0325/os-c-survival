# Day21：/proc 與 COW（用 smaps_rollup 看見 fork 的共享與複製）

專案：os-c-survival  
進度：已完成 day20  
日期：2026-02-19（Asia/Taipei）

本日目標：把「process / memory / COW」從抽象概念落地到 Linux `/proc` 的真實資料。

---

## 今日重點

1. 熟悉 `/proc/<pid>/` 與 `/proc/self/` 的常用檔案  
2. 寫出一個小工具 `proc_inspect`：從 `/proc/.../status` 擷取關鍵欄位  
3. 寫出 `proc_fork_compare`：用 fork + 32MB 匿名記憶體示範 Copy-on-Write  
4. 使用 `/proc/<pid>/smaps_rollup`，觀察 `Shared_* / Private_* / Pss` 的變化，**用數字證明 COW**

---

## 檔案結構

```
day21/
  proc_inspect.c
  proc_fork_compare.c
  README.md
```

---

## 編譯指令與旗標解釋

### 編譯
```bash
gcc -O0 -g -Wall -Wextra -o day21/proc_inspect day21/proc_inspect.c
gcc -O0 -g -Wall -Wextra -o day21/proc_fork_compare day21/proc_fork_compare.c
```

### 旗標解釋
- `-O0`：關閉最佳化，方便除錯與對照程式行為（最佳化可能改變變數/程式結構，讓觀察變困難）
- `-g`：加入除錯資訊（讓 gdb 能對應到原始碼行號、變數）
- `-Wall -Wextra`：開啟更多編譯器警告，提前發現可疑行為或未定義行為

---

## Part 1：proc_inspect（讀 /proc/*/status）

### 目標
從 `/proc/self/status`（或指定 pid 的 `/proc/<pid>/status`）擷取常用欄位：

- Name, State, Tgid, Pid, PPid, Threads
- VmSize, VmRSS
- voluntary_ctxt_switches, nonvoluntary_ctxt_switches

### 使用方法
```bash
./day21/proc_inspect
./day21/proc_inspect --pid 306
./day21/proc_inspect --pid $$
```

### 觀察重點
- `VmSize`：虛擬位址空間大小（不代表都在 RAM）
- `VmRSS`：resident set size，表示目前在 RAM 的頁面總量（不分 shared/private）
- `voluntary_ctxt_switches`：主動讓出 CPU（sleep、wait、blocking I/O 常見）
- `nonvoluntary_ctxt_switches`：被動被切走（時間片用完、優先權等）

---

## Part 2：proc_fork_compare（COW 示範）

### 實驗設計
1. parent 先 `malloc 32MB`，並「每頁寫 1 byte」(4096 bytes step) 讓頁面真的進入 RSS  
2. fork 之後：
   - child 先讀一次 `smaps_rollup`（此時理論上仍共享，屬於 COW 的共享階段）
   - child 對每頁再寫一次，觸發 COW，把共享頁複製成 private
3. 用 `smaps_rollup` 觀察 shared/private 與 Pss 的變化

### 執行
```bash
./day21/proc_fork_compare
```

### 一次成功的輸出（節錄）

```text
=== proc_fork_compare (COW demo) ===
[parent:after_touch_before_fork] ... VmRSS=34048 kB
[parent:after_touch_before_fork] rollup: Rss=34412 kB Pss=32931 kB  SC=1536 SD=0     PC=12 PD=32864

[parent:after_fork] ...
[parent:after_fork] rollup:          Rss=34540 kB Pss=16519 kB  SC=1672 SD=32824 PC=4  PD=40

[child:after_fork_before_write] ...
[child:after_fork_before_write] rollup: Rss=33752 kB Pss=16486 kB  SC=888  SD=32824 PC=0  PD=40

[child:after_cow_write] ...
[child:after_cow_write] rollup:        Rss=33752 kB Pss=32870 kB  SC=888  SD=56    PC=0  PD=32808

[parent:after_wait] ...
[parent:after_wait] rollup:            Rss=34540 kB Pss=32936 kB  SC=1664 SD=0     PC=12 PD=32864
```

---

## 如何讀懂 smaps_rollup（本日最重要）

`/proc/<pid>/smaps_rollup` 提供整個 process 的 memory 匯總（比 status 更能看 shared/private）：

- `Rss`：總 resident pages（不分 shared/private）
- `Pss`：Proportional Set Size，共享頁會按共享者數量分攤
- `Shared_Clean / Shared_Dirty`：共享頁（乾淨/髒）
- `Private_Clean / Private_Dirty`：私有頁（乾淨/髒）

### 現象 1：fork 後，Pss 砍半（共享階段）
在 `parent:after_fork` 與 `child:after_fork_before_write`：

- `SD`（Shared_Dirty）突然變成約 32MB  
- `PD`（Private_Dirty）掉到很小  
- `Pss` 約為 16MB（≈ 32MB / 2）

解釋：fork 之後匿名頁進入 COW 共享狀態，兩個 process 映射到同一批 physical pages，Pss 因分攤而下降。

### 現象 2：child 寫入後，Shared → Private（COW 觸發）
在 `child:after_cow_write`：

- `SD` 從 ~32MB 大幅下降
- `PD`（Private_Dirty）上升到 ~32MB
- `Pss` 從 ~16MB 回到 ~32MB

解釋：child 寫入每一頁會觸發 COW，kernel 會複製頁面，讓 child 得到自己的 private pages；因此共享量下降、私有量上升。

### 為什麼只看 VmRSS 不明顯？
`VmRSS` / `Rss` 只告訴你「有多少 resident」，不區分 shared/private。  
COW 前後映射到的 resident 頁數量都差不多，因此 `VmRSS` 可能幾乎不變。  
要看 COW 的本質，應該看 `Shared_* / Private_* / Pss`。

---

## 手動查 smaps_rollup（輔助方式）

程式印出的 pid 可用來手動查：

```bash
cat /proc/<PARENT_PID>/smaps_rollup | egrep 'Rss:|Pss:|Shared_|Private_'
cat /proc/<CHILD_PID>/smaps_rollup  | egrep 'Rss:|Pss:|Shared_|Private_'
```

---

## 小結

- `/proc/.../status` 適合快速看 process 的基本屬性（PID/PPID/Threads/VmSize/VmRSS/context switch）
- `smaps_rollup` 才能把共享與私有拆開，並用 `Pss` 看共享分攤
- fork 後的匿名頁會進入 COW 共享狀態：`Shared_*` 上升、`Pss` 下降
- child 寫入後觸發 COW：`Private_Dirty` 上升、`Shared_*` 下降、`Pss` 回升

---