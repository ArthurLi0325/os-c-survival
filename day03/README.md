# OS-C-Survival — Day03  
## Control Flow, Error Handling, and Debugging

### 今日目標
Day03 的重點不在於「寫出功能」，而是學會如何讓程式：
- 行為可控
- 錯誤可預期
- 問題可定位

也就是從「會跑的 C 程式」進化到「可維護的系統程式」。

---

## 一、錯誤處理策略的分工

在系統程式中，不是所有錯誤都該用同一種方式處理。  
本日實作中，我們明確區分了三種錯誤類型，並對應不同處理手段。

### 1. die()：不可恢復的 Fatal Error

適用情境：
- 記憶體配置失敗（malloc / calloc / realloc）
- 系統資源無法取得
- 工具型程式或初始化階段錯誤

策略：
- 印出一致格式的錯誤訊息
- 直接結束程式

```c
__attribute__((noreturn))
static void die(const char *fmt, ...) {
    int saved_errno = errno;

    va_list ap;
    va_start(ap, fmt);

    fprintf(stderr, "fatal: ");
    vfprintf(stderr, fmt, ap);

    if (saved_errno != 0) {
        fprintf(stderr, ": %s", strerror(saved_errno));
    }
    fprintf(stderr, "\n");

    va_end(ap);
    exit(1);
}
```

---

### 2. assert()：程式員錯誤（Invariant）

assert 用來保護「理論上不應該發生的狀態」，例如：
- 傳入的指標為 NULL
- 陣列長度為 0
- 內部邏輯假設被破壞

特性：
- 僅在 debug build 中啟用
- release build（NDEBUG）會被移除
- 不可用於必要的 runtime 錯誤處理

```c
assert(a != NULL);
assert(n > 0);
```

---

### 3. return value + errno：可恢復錯誤

對於可能由外部輸入造成的錯誤：
- 不應直接 abort
- 應回傳錯誤狀態，交由呼叫者處理

```c
if (n == 0) {
    errno = EINVAL;
    return -1;
}
```

---

## 二、安全記憶體配置包裝（xmalloc 系列）

為避免在每個呼叫點重複錯誤處理，本日實作了強制成功或死亡的配置函式。

### xmalloc
```c
void* xmalloc(size_t n) {
    void *p = malloc(n);
    if (!p) die("xmalloc(%zu) failed", n);
    return p;
}
```

### xcalloc
- 保證配置後內容初始化為 0

### xrealloc
- 使用暫存指標避免 realloc 失敗造成原指標遺失

```c
void* xrealloc(void *ptr, size_t newsize) {
    void *tmp = realloc(ptr, newsize);
    if (!tmp) die("xrealloc(%p, %zu) failed", ptr, newsize);
    return tmp;
}
```

---

## 三、Debug 流程（assert + gdb）

1. 使用 assert 製造可預期 crash  
2. 使用 gdb 定位錯誤位置

常用指令：
```gdb
bt
frame N
list
print variable
up
```

重點觀念：
- stack trace 底層多為 libc
- 真正該看的通常是最後幾層 user code
- debug 的本質是驗證假設，而非猜測

---

## 四、總結

Day03 建立了以下關鍵觀念：
- 明確區分錯誤類型
- assert、die、return/errno 各司其職
- 錯誤處理應集中管理
- 使用工具精準定位問題

這些原則將在後續 stack / heap / lifetime / UB 中持續使用。