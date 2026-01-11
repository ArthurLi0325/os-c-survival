# Day02 – Memory Model & Pointer Survival

> 專案：**os-c-survival**  
> 進度：**Day02 完成**  
> 主題：**C 語言的記憶體模型、pointer、stack 與 heap 的生存法則**

---

## 今日目標

Day02 的核心目標不是「記住規則」，而是：

> **親眼看見 pointer 指向的記憶體發生了什麼事，以及它什麼時候「死亡」**

透過實際撰寫程式與 gdb / AddressSanitizer 觀察，我們要回答三個根本問題：

1. C 程式中的資料實際上存在哪裡？（stack / heap / global / code）
2. pointer 指向的「位址」什麼時候是合法的？
3. 為什麼 Undefined Behavior 這麼危險、這麼難 debug？

---

## C 程式的記憶體版圖（Memory Layout）

一個典型的 C 程式，在 OS 眼中大致分成以下區域：

```
[ Code / Text Segment ]   ← function、本體程式碼
[ Global / Static Data ]  ← global / static 變數
[ Heap ↓ ]                ← malloc / free 管理
--------------------
[ Stack ↑ ]               ← function 呼叫、local variable
```

- **Stack**：
  - 每次 function 呼叫時建立 stack frame
  - 存放 local variable、return address、暫存狀態
  - function return 時，整個 frame 立刻失效

- **Heap**：
  - 由 allocator（如 glibc malloc）管理
  - 生命週期不依附於 function
  - 是否「活著」取決於是否已被 `free()`

---

## 程式一：`mem_layout.c`

### 功能說明

此程式用來**實際印出不同類型資料的位址**，驗證它們位於不同記憶體區段。

```c
int global_var = 123;

int main(void) {
    int local_var = 456;
    int *heap_var = malloc(sizeof(int));
    *heap_var = 789;

    printf("&global_var = %p\n", &global_var);
    printf("&local_var  = %p\n", &local_var);
    printf("heap_var    = %p\n", heap_var);
    printf("main        = %p\n", main);
}
```

### 觀察重點

- `&local_var` 通常位於 **高位址（stack）**
- `heap_var` 位於 **較低位址（heap）**
- `&global_var` 與 `main` 位於 **程式映像區域**

### 學到的事

> **變數的「型態」不決定它在哪，
> 決定的是它的「生命週期與配置方式」**

---

## 程式二：`return_local.c` —— stack 生命週期

### 程式內容（簡化）

```c
int *foo(void) {
    int x = 10;
    return &x;   // ❌ 回傳 stack 上的位址
}

int main(void) {
    int *p = foo();
    printf("%d\n", *p);
}
```

### `foo()` 的意義

- `foo()` 是一個**用來展示 stack frame 生命週期的實驗函式**
- `x` 是 `foo()` 的 local variable，存在於 `foo` 的 stack frame 中

### 為什麼這段程式是錯的？

- `foo()` return 時：
  - 它的 stack frame **立刻被銷毀**
  - `x` 的位址不再對應任何合法物件

- `main()` 中的 `p`：
  - 只是「一個數值相同的位址」
  - 但**指向的物件已經死亡**

### 重點結論

> **Stack 上的資料，生命週期嚴格受限於 function 呼叫**

這也是為什麼 stack 非常適合：
- function 的暫存資料
- 不需要跨越呼叫邊界的狀態

---

## 程式三：`use_after_free.c` —— heap 所有權

### 程式內容（簡化）

```c
int *p = malloc(sizeof(int));
*p = 42;
free(p);
*p = 99;   // ❌ use-after-free
```

### `malloc()` 的意義

- `malloc()` 向 allocator **請求一塊 heap 記憶體**
- 回傳值是一個 pointer，代表「你目前擁有這塊記憶體的使用權」

### `free()` 真正做了什麼？

- **不是清空記憶體**
- 而是：

> **告訴 allocator：我放棄這塊記憶體的所有權**

### 為什麼 free 之後還能讀到值？

- 那只是 allocator **暫時還沒重用那塊記憶體**
- 並不代表行為是正確的

### 關鍵結論

> **Heap 的正確性不取決於值是否還在，而取決於你是否仍然擁有它**

---

## 程式四：`uaf_reuse.c` —— dangling pointer aliasing

### 行為說明

此程式展示：

- `free(p)` 後
- 下一個 `malloc()` 可能直接重用同一塊位址

```c
free(p);
int *q = malloc(sizeof(int));
```

結果：
- `p` 與 `q` 指向 **同一塊 heap**
- `p` 成為「已死亡但仍能讀寫的別名」

### 這代表什麼？

> **dangling pointer 可能偷偷變成別的物件的 alias**

這正是 production crash 與資安漏洞的來源。

---

## 工具：AddressSanitizer（ASan）

使用編譯選項：

```bash
gcc -fsanitize=address -fno-omit-frame-pointer ...
```

ASan 的作用是：

- 在 `free()` 後把記憶體標記為「毒區」
- 任何 read / write 立刻中止程式並報錯

### 為什麼要用 ASan？

> **C 語言允許你犯錯，但 ASan 會在犯錯的瞬間抓住你**

---

## 為什麼在記憶體階層討論中常出現 `foo()` 與 `malloc()`？

因為它們剛好代表了兩種完全不同的記憶體模型：

| 元素 | foo() | malloc() |
|----|----|----|
| 所在區域 | Stack | Heap |
| 生命週期 | function scope | 手動管理 |
| 失效時機 | return | free |
| 常見錯誤 | return local pointer | use-after-free |

這兩者是理解 C 記憶體模型的**最小對立組合**。

---

## Day02 總結

1. pointer ≠ object
2. stack 的問題是「生命週期」
3. heap 的問題是「所有權」
4. free 不是清空，是放棄主權
5. Undefined Behavior 不會立刻爆，但一定會害你

---

