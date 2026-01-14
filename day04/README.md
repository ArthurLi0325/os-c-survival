# Day04 – Stack, Heap, and Function Call Frames

## 專案
os-c-survival

## 今日進度
完成 Day04：深入理解 stack / heap 的生命週期差異，並透過實驗驗證 function call frame 的實際行為。

---

## 今日目標

- 理解 stack 與 heap 的本質差異
- 觀察 function call 時 stack frame 的成長方向與大小
- 理解為什麼不能回傳 local variable（stack）位址
- 學會正確回傳指標的幾種方式與其代價

---

## 編譯環境與參數說明

今日所有程式皆使用以下指令編譯：

```bash
gcc -O0 -g -Wall -Wextra <source.c> -o <output>
```

### 為什麼要這樣編譯？

- `-O0`  
  關閉最佳化，避免 compiler 省略 stack frame 或 inline 函式  
  → 有助於觀察真實的 function call 與 stack 行為  

- `-g`  
  產生 debug symbols  
  → 之後可使用 gdb 觀察 stack pointer、frame pointer 與 call stack（Day05 會用到）  

- `-Wall`  
  開啟常見警告（如型別錯誤、未使用變數）  

- `-Wextra`  
  開啟更多額外警告（如錯誤的 printf 格式）  

---

## Stack 成長方向與 Stack Frame 實驗

### 實驗檔案
- `stack_addr_recur.c`

### 實驗方式
使用遞迴函式，在每一層呼叫中宣告 local variable，並印出其位址。

### 觀察結果（實際輸出）

```text
depth=5 &x=0x7fff05c6c874
depth=4 &x=0x7fff05c6c844
depth=3 &x=0x7fff05c6c814
...
```

### 結論
- 每次遞迴呼叫，`&x` 位址往低位址移動
- 在此系統（x86-64 / WSL）中，stack **往低位址成長**
- 相鄰兩層差距為 `0x30 = 48 bytes`

---

### Stack Frame 大小可變實驗

在同一個程式中新增：

```c
char buf[64];
```

重新觀察後得到：

```text
depth=5 &x=...
depth=4 &x=...
```

- 相鄰 stack frame 差距變為 `0x80 = 128 bytes`

#### 說明
- stack frame 內不只有 local variables
- 還包含 return address、saved registers、alignment padding
- frame 大小會隨 local data 與 ABI 規範改變

---

## 回傳指標的三種情況（重點）

### ❌ 回傳 local（stack）變數位址
檔案：`ret_local_addr.c`

```c
int* foo() {
    int x = 10;
    return &x;
}
```

#### 結果
- 編譯時出現 warning
- 執行時可能：
  - `p = NULL`
  - segmentation fault
  - 或其他不可預期行為

#### 原因
- `x` 位於 `foo()` 的 stack frame
- 函式 return 後 stack frame 結束
- 回傳的位址成為 dangling pointer

> 結論：**不能回傳 stack 上 local variable 的位址**

---

### ✅ 回傳 heap（malloc）位址
檔案：`ret_heap_addr.c`

```c
int* foo() {
    int *p = malloc(sizeof(int));
    *p = 10;
    return p;
}
```

#### 特性
- 記憶體配置於 heap
- 函式 return 後仍然存在
- 必須由某一方負責 `free`

> 結論：  
> **要跨函式存活 → 用 heap；但一定要記得 free**

---

### ✅ 回傳 static 變數位址
檔案：`ret_static_addr.c`

```c
int* foo() {
    static int x = 10;
    return &x;
}
```

#### 實驗結果
- 每次呼叫回傳同一個位址
- 修改後會影響後續呼叫

```text
in foo: x=10
after modify: x=99
```

#### 特性與代價
- 不會因 stack frame 結束而消失
- 狀態會被共享
- 不適合需要獨立結果或多執行緒情境

> 結論：  
> **static 可回傳，但它是共享狀態**

---

## 今日總結（Day04 心智模型）

- function call 會建立 stack frame
- stack frame 在 return 後即不再保證存在
- 回傳指標時，必須確認記憶體的生命週期
- stack / heap / static 的選擇，決定了程式是否正確

### 一句話總結
> **C 程式的正確性，來自於你是否理解記憶體的生命週期，而不是語法本身。**

---