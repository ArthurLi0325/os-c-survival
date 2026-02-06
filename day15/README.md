# Day15：MAP_PRIVATE 與 Copy-On-Write（COW）

本日重點在於理解 **fork 之後，MAP_PRIVATE 記憶體是如何在「第一次寫入時」才發生實體頁複製**，
並且透過 `/proc/<pid>/smaps` 取得**可量化的證據**，而非只停留在概念層級。

---

## 一、學習目標

- 理解 `MAP_PRIVATE` 與 `fork()` 的互動行為
- 理解 Copy-On-Write（COW）實際發生的時機
- 能使用 `/proc/<pid>/smaps` 驗證共享頁與私有頁的變化
- 能說明 `Rss / Pss / Shared_Dirty / Private_Dirty` 的意義
- 使用 `pipe()` 做程序同步，排除排程造成的觀察歧義

---

## 二、核心觀念整理

### 1. MAP_PRIVATE 的真正語意
- fork 後：
  - **讀取時仍共享實體頁**
  - **第一次寫入才觸發 COW**
- fork 本身不會立刻複製實體頁，只複製頁表結構

### 2. Copy-On-Write（COW）
- 多個行程初期共用同一實體頁
- 任一行程第一次寫入該頁：
  - 內核複製一份新頁
  - 寫入行程取得新頁（Private）
  - 其他行程保留舊頁

---

## 三、實驗一：MAP_PRIVATE + fork + child 寫入

### 程式：`cow_mmap_private.c`

- 使用 `mmap(MAP_PRIVATE | MAP_ANONYMOUS)`
- fork 後由 child 進行第一次寫入
- parent 在 child 寫入後再次讀取

### 預期行為
- child 寫入後：
  - child 的 `Private_Dirty` 增加
- parent：
  - 讀到的值維持不變
  - 不會看到 child 的修改

---

## 四、實驗二：加入同步，避免排程干擾

### 為什麼需要同步？
若不同步，parent 與 child 的輸出順序可能因排程而交錯，
造成「看起來像是 parent 先讀、child 才寫」的錯覺。

### 解法：使用 `pipe()`
- parent：
  - `read()` pipe，阻塞等待
- child：
  - 寫入完成後 `write()` pipe 通知 parent

此設計可保證：
- parent 的讀取 **一定發生在 child 寫入之後**

### 程式：`cow_mmap_private_sync.c`

---

## 五、smaps 驗證（關鍵證據）

### 寫入前（parent / child 相同）

```
Rss:           8 kB
Pss:           4 kB
Shared_Dirty:  8 kB
Private_Dirty: 0 kB
```

說明：
- 2 個 page 在 RAM
- 2 個 page 皆為共享（COW 尚未觸發）
- PSS 為共享頁平均分攤

---

### 寫入後（parent / child 皆出現 Private_Dirty）

```
Rss:           8 kB
Pss:           6 kB
Shared_Dirty:  4 kB
Private_Dirty: 4 kB
```

說明（以 page 為單位）：
- 原本 2 個共享頁：
  - 其中 1 頁被 child 寫入 → 觸發 COW
- 結果：
  - child：取得新頁（Private_Dirty +4kB）
  - parent：舊頁只剩自己使用 → 也記為 Private_Dirty
  - 剩下 1 頁仍為共享頁 → Shared_Dirty 4kB
- PSS = 4kB（私有） + 2kB（共享頁的一半） = 6kB

---

## 六、為什麼 parent 的 Private_Dirty 也會增加？

這是 COW 的常見誤解點：

- child 觸發 COW 後，不再使用舊實體頁
- 舊實體頁只剩 parent 一個行程使用
- 內核因此將該頁記帳為 **parent 的 Private_Dirty**

這並不是 parent 主動寫入，而是「共享關係消失」導致的結果。

---

## 七、本日總結

- fork 並不會立刻複製實體記憶體
- `MAP_PRIVATE` + fork 依賴 COW 延遲複製成本
- COW 只在第一次寫入時發生
- `/proc/<pid>/smaps` 可提供可量化、可驗證的證據
- 使用同步機制（如 pipe）才能做出嚴謹的實驗結論

---

## 八、關鍵一句話結論

> child 第一次寫入 MAP_PRIVATE 映射時觸發 COW，  
> child 取得新私有頁，而 parent 保留舊頁；  
> 共享關係解除後，舊頁對 parent 亦記為 Private_Dirty，  
> 剩餘頁仍共享，因此 PSS 與 Shared_Dirty 同步變化。

