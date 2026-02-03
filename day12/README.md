# Day12：Process Memory Map（/proc/<pid>/maps）與大塊 malloc 的來源

## 目標
1. 透過 `/proc/<pid>/maps` 觀察一個 process 的虛擬位址空間由哪些區段組成。
2. 將程式中印出的位址（global / static / stack / heap / rodata）對回 maps 的區段。
3. 以實驗方式驗證：大塊 `malloc` 通常不是擴張 `[heap]`，而是使用匿名 `mmap` 映射。

---

## 1. mem_layout：把位址對回 maps

### 1.1 程式行為說明
程式中同時配置並印出以下類型的位址：
- 已初始化的 global / static 變數
- 未初始化的 global / static 變數
- local 變數
- `malloc` 配置的指標
- 字串常量（string literal）

### 1.2 實際觀察結果（對應 `/proc/<pid>/maps`）
- **global / static（含 local static）**：
  落在程式自身映射的 `rw-p` 區段，對應 `.data` 與 `.bss`。
- **local 變數**：
  位址落在 `[stack]` 區段。
- **小塊 `malloc`**：
  通常位於 `[heap]` 區段（由 `brk` 成長）。
- **字串常量**：
  位於程式映射的 `r--p` 區段，對應唯讀資料（`.rodata`）。

### 1.3 maps 權限欄位解讀
- `r-xp`：可讀、可執行（通常是 `.text`，程式碼）
- `r--p`：只讀（`.rodata` 或 ELF metadata）
- `rw-p`：可讀可寫（`.data` + `.bss`）
- `p`：private mapping（copy-on-write，不直接回寫檔案）

---

## 2. heap_growth：觀察大塊 malloc 的實際來源

### 2.1 實驗設定
- 每次呼叫 `malloc(1 MiB)`
- 總共配置 20 次（約 20 MiB）
- 在配置完成後，暫停程式以便從另一個 terminal 觀察 `/proc/<pid>/maps` 與 `pmap`

### 2.2 關鍵觀察
1. **`[heap]` 區段大小不變**
   - 例如：
     ```
     62d6d9dbc000-62d6d9ddd000 rw-p ... [heap]
     ```
   - 即使完成 20 次 1 MiB 的 `malloc`，`[heap]` 範圍仍未擴張。

2. **process 的總記憶體顯著增加**
   - `pmap -x <pid>` 顯示：
     ```
     total kB 23240
     ```
   - 與未配置前（僅數 MB）相比，明顯增加。

3. **出現大型匿名映射段 `[ anon ]`**
   - 例如：
     ```
     0x7e20dd0ed000   19532 kB   rw---   [ anon ]
     0x7e20de689000    1040 kB   rw---   [ anon ]
     ```
   - 合計約 20572 kB，與 20 MiB 的配置量級一致（加上對齊與 allocator 管理開銷）。

4. **`malloc` 回傳的指標位址**
   - 實際回傳的 pointer（例如 `0x7e20dd0ed010`）落在上述大型 `[ anon ]` 映射段內。

### 2.3 結論
- 大塊 `malloc`（例如 1 MiB 等級）通常不會透過 `brk` 擴張 `[heap]`。
- glibc 的 allocator 會改用 `mmap` 建立匿名映射段，因此在 `/proc/<pid>/maps` 與 `pmap` 中顯示為 `[ anon ]`。
- 這種設計有助於：
  - 大塊記憶體較容易歸還給 OS
  - 避免 heap 長期膨脹與碎片化

---

## 編譯與執行

### mem_layout
```bash
gcc -O0 -g -Wall -Wextra -o day12/mem_layout day12/mem_layout.c
./day12/mem_layout
```

### heap_growth
```bash
gcc -O0 -g -Wall -Wextra -o day12/heap_growth day12/heap_growth.c
./day12/heap_growth
```

### 觀察指令
```bash
grep -E "\[heap\]|\[stack\]|mem_layout|heap_growth" /proc/<pid>/maps
pmap -x <pid> | sort -k2 -n | tail -n 25
```

---

## 備註
- 每次執行程式時，實際位址與 PID 可能不同（ASLR 的正常行為）。
- 位址不同不影響結論，只要觀察到：
  - `[heap]` 未成長
  - 大型 `[ anon ]` 映射出現
  即可確認大塊 `malloc` 走 `mmap` 的行為。

