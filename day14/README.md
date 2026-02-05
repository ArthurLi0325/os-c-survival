# Day14：fork + mmap 與 Copy-on-Write（COW）

## 本日目標
透過實驗理解 fork 之後記憶體是否真的被複製，以及 MAP_PRIVATE 與 MAP_SHARED 在 mmap 下的實際差異，並使用 /proc/<pid>/maps、smaps 驗證核心行為。

---

## 實驗一：heap（malloc）+ fork

### 實驗方式
- 在 fork 前於 heap 上配置一個整數並初始化
- fork 後由子行程修改該值
- 父行程再次讀取該值
- 使用 smaps 觀察 Private_Dirty

### 結果
- 父子行程的 heap 虛擬位址相同
- 子行程寫入後，父行程讀到的值不變
- 子行程 heap 出現 Private_Dirty（以 page 為單位）

### 結論
heap 在 fork 後採用 Copy-on-Write，寫入才會觸發實體 page 複製。

---

## 實驗二：mmap(MAP_PRIVATE | MAP_ANONYMOUS) + fork

### 實驗方式
- 使用 mmap 建立 private 的匿名映射
- fork 後由子行程寫入
- 父行程讀取值並觀察 smaps

### 結果
- 父行程仍讀到原值
- 子行程 smaps 顯示 Private_Dirty
- VMA 權限為 rw-p

### 結論
MAP_PRIVATE 的 mmap 行為與 heap 相同，皆為 Copy-on-Write。

---

## 實驗三：mmap(MAP_SHARED | MAP_ANONYMOUS) + fork

### 實驗方式
- 使用 mmap 建立 shared 的匿名映射
- fork 後由子行程寫入
- 父行程再次讀取

### 結果
- 父行程立即讀到子行程寫入的新值
- 父子 smaps 皆顯示 Shared_Dirty，無 Private_Dirty
- VMA 權限為 rw-s

### 結論
MAP_SHARED 代表真正的共享實體 page，父子行程對同一 page 的修改彼此可見。

---

## 總結比較

| 類型 | maps 權限 | 子寫後父是否看到 | smaps 指紋 |
|---|---|---|---|
| heap (malloc) | rw-p | 否 | 子行程出現 Private_Dirty |
| mmap MAP_PRIVATE | rw-p | 否 | 子行程出現 Private_Dirty |
| mmap MAP_SHARED | rw-s | 是 | Shared_Dirty，Private_Dirty 為 0 |

---

## 核心結論

- fork 並不會立即複製記憶體內容
- Linux 透過 Copy-on-Write 延遲實體 page 複製
- 是否共享，取決於 VMA 屬性（private / shared）
- smaps 是觀察 COW 與共享行為最直接的工具
