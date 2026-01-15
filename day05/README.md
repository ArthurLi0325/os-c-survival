# Day05 — Stack Frame、Function Call 與 Undefined Behavior（UB）

專案：**os-c-survival**

---

## 今日目標

理解 C 語言中「函式呼叫」在 **OS / ABI / CPU** 層級實際是如何運作的：

- Stack frame 是怎麼建立與重用的
- 為什麼回傳 local variable 的位址是 **Undefined Behavior（UB）**
- `call / ret` 指令如何配合 stack 運作

Day05 的重點不是語法，而是把 **C 程式行為對齊到機器真相**。

---

## Part 1：Stack frame 的配置與重用

### 位址觀察

透過印出 local variable 與 function parameter 的位址，我們觀察到：

- `x`（參數）、`a`、`b`（local variables）
- 位址彼此非常接近，代表它們位於同一個 **stack frame** 中

當同一個 function 被連續呼叫兩次時：

- `&x`、`&a`、`&b` 的位址 **完全相同**

> 這直接證明：**stack frame 在 function return 之後會被重用**。

Stack 不是「被清空」，而是變成「可再使用的空間」。

---

## Part 2：為什麼回傳 local 位址是 UB

### 錯誤示範

```c
int* bad_return(void) {
    int x = 999;
    return &x;  // ❌ Undefined Behavior
}
```

### 實際觀察到的行為

- 有時回傳的指標是 `NULL`
- 有時指向已被其他 function 使用的 stack 記憶體
- 有時直接 segmentation fault

這些 **全部都是合法結果**，因為：

> C 語言對這種行為「不提供任何保證」。

**Undefined Behavior 不是「偶爾錯」**，而是「任何事情都可能發生」。

---

## Part 3：使用 AddressSanitizer（ASan）觀察 UB

編譯方式：

```bash
gcc -O0 -g -fsanitize=address -fno-omit-frame-pointer
```

ASan 清楚顯示：

- 我們使用了已經失效生命週期的記憶體
- Crash 發生於非法記憶體存取（常見為 NULL 解引用）

ASan 的價值在於：

> 把「看似能跑」的 UB，轉成 **明確、可除錯的錯誤**。

---

## Part 4：三種正確回傳資料的方法

### A）Heap（`malloc`）

```c
int* good_heap(void) {
    int *p = malloc(sizeof(int));
    *p = 999;
    return p;   // caller 必須 free
}
```

- 生命週期：直到 `free()`
- Ownership：caller
- 彈性高，但需要嚴格管理

---

### B）Static storage

```c
int* good_static(void) {
    static int x = 999;
    return &x;
}
```

- 生命週期：整個程式期間
- 位址固定、簡單
- 缺點：
  - 多次呼叫共用同一份資料
  - 預設非 thread-safe

---

### C）Out-parameter（caller 提供儲存空間）

```c
void good_outparam(int *out) {
    *out = 999;
}
```

- 生命週期完全由 caller 控制
- 無動態配置
- **OS / embedded 系統程式最常用的風格**

---

## Part 5：使用 gdb 觀察 stack frame

工具與編譯設定：

```bash
gcc -O0 -g -fno-omit-frame-pointer
```

常用指令：

```gdb
bt
info frame
info registers rbp rsp rip
```

### 我們確認的事實

- 每次 function call 都會建立一個 stack frame
- frame 內至少包含：
  - saved `rbp`
  - saved `rip`（return address）
- 連續兩次呼叫同一個 function，`rbp` / `rsp` 完全相同

> 這是 **stack frame 被重用** 的機器層級證據。

---

## Part 6：用 objdump 驗證 call / ret 機制

透過：

```bash
objdump -d -Mintel ./gdb_stack_frame
```

在 `main` 中找到兩個呼叫 `foo` 的指令：

```asm
1298: call 1189 <foo>
12a2: call 1189 <foo>
```

在 x86-64 架構下，`call rel32` 指令長度為 **5 bytes**：

- 第一次呼叫 return address：`0x1298 + 5 = 0x129d`
- 第二次呼叫 return address：`0x12a2 + 5 = 0x12a7`

這與 gdb `info frame` 顯示的 `saved rip` 完全一致。

> **saved rip = call 指令的下一條指令位址**

`ret` 指令只是把這個位址從 stack pop 回 `rip`。

---

## 今日重點總結（Key Takeaway）

> **Stack 記憶體只在 function 還活著時有效。**

一旦 function return，其 stack frame 立刻變成可重用空間。
因此，回傳 local variable 的位址必然導致 **Undefined Behavior**。

若資料需要跨 function 存活，正確選擇是：

- Heap（`malloc`，並清楚定義誰負責 `free`）
- Static storage（整個程式生命週期）
- Caller-provided storage（out-parameter，系統程式首選）

Day05 的核心，是把 C 語言行為與 OS / ABI / CPU 真實執行模型完全對齊