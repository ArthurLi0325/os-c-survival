# Day13：Process Memory 與 mmap

## 今日目標

本日目標是從 OS 的角度理解「行程的記憶體是如何被建立與管理的」，並實際透過 `mmap` 驗證：

- 記憶體不只來自 heap（brk / sbrk）
- `mmap` 是 OS 提供的另一條重要配置路徑
- `/proc/<pid>/maps` 反映的是 kernel 的記憶體對映結果
- `MAP_SHARED` 與 `MAP_PRIVATE` 的差異並非抽象概念，而是能被實驗驗證的行為

---

## 程式一：匿名 mmap（mmap_anon.c）

### 設計目的

此程式用來驗證「不透過 heap，也能直接向 OS 要一段記憶體」。

- 使用 `MAP_ANONYMOUS`
- 不對應任何檔案
- 驗證該區段會出現在 `/proc/<pid>/maps`，但沒有 pathname

### 關鍵行為

- 呼叫 `mmap` 配置一個 page（4096 bytes）
- 寫入資料，確保 page 實際被使用
- 在程式暫停時，觀察 `/proc/<pid>/maps`

### 實驗觀察

- 該 mapping 顯示為：

```
xxxx-xxxx rw-p 00000000 00:00 0
```

- 沒有檔名（anonymous）
- 權限為 `rw-p`，代表 private mapping
- 不屬於 `[heap]`，位置可出現在任意位址

這證明：

> mmap 是 OS 直接建立的一段 virtual memory mapping，不是 heap 的延伸。

---

## 程式二：檔案 mmap（mmap_file.c）

### 設計目的

驗證「檔案可以被映射成記憶體」，以及 `MAP_SHARED` / `MAP_PRIVATE` 對寫入行為的影響。

### 實驗流程

1. 使用 `open(O_RDWR | O_CREAT | O_TRUNC)` 建立檔案
2. 使用 `ftruncate` 將檔案擴展至 4096 bytes
3. 使用 `mmap` 對應檔案
4. 對映射的記憶體進行寫入
5. 從檔案內容與 `/proc/<pid>/maps` 驗證結果

### 為什麼一定要 ftruncate

若檔案實際大小小於 mmap 長度：

- 寫入超出檔案大小的區段會觸發 `SIGBUS`

因此在 mmap 前，必須先確保 backing file 的長度覆蓋映射範圍。

---

## MAP_SHARED：寫入會影響檔案

使用：

```c
mmap(..., MAP_SHARED, fd, 0);
```

### maps 觀察結果

```
xxxx-xxxx rw-s ... day13/demo.txt
```

- `s` = shared
- 記憶體的修改會進入 page cache
- 最終會同步回檔案（本實驗中亦可使用 `msync` 強制同步）

### 實驗結果

- `head demo.txt` 可直接看到寫入的字串
- `hexdump` 顯示檔案內容確實被修改

---

## MAP_PRIVATE：Copy-on-Write，不改檔案

使用：

```c
mmap(..., MAP_PRIVATE, fd, 0);
```

### maps 觀察結果

```
xxxx-xxxx rw-p ... day13/demo.txt
```

- `p` = private
- 寫入會觸發 Copy-on-Write
- 修改只存在於該 process 的 private page

### 實驗結果

- 程式內讀取 memory 時可看到寫入內容
- `head demo.txt` 無輸出（內容仍為 0x00）
- `hexdump` 顯示檔案未被修改

即使呼叫 `msync`，在 MAP_PRIVATE 下也不保證會寫回檔案。

---

## rw-p 與 rw-s 的意義總結

| 權限 | 意義 | 寫入效果 |
|----|----|----|
| rw-p | private | 僅影響該 process（COW） |
| rw-s | shared | 可能影響檔案或其他 process |

heap、stack、匿名 mmap 預設皆為 `rw-p`。

---

## 編譯選項說明

本日程式皆使用以下編譯選項：

```bash
gcc -O0 -g -Wall -Wextra
```

- `-O0`：關閉最佳化，方便除錯與對照位址
- `-g`：產生除錯資訊，供 gdb 使用
- `-Wall -Wextra`：開啟額外警告，避免潛在錯誤

---

## 本日結論

- Process 的記憶體是由 OS 透過多種 mapping 建立，而非只有 heap
- `mmap` 是 OS 級別的記憶體配置介面
- `/proc/<pid>/maps` 反映 kernel 真實管理狀態
- `MAP_SHARED` 與 `MAP_PRIVATE` 的差異可以被實驗直接驗證

至此，已完成從「C 語言記憶體操作」到「OS 虛擬記憶體對映模型」的轉換。