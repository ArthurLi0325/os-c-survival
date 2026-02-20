# Day22 --- ELF 與 Linux 記憶體管理機制

專案：os-c-survival\
進度：已完成 Day21\
主題：ELF 載入機制、.bss、Demand Paging、mmap vs brk、RSS 行為分析

------------------------------------------------------------------------

## 一、今日核心目標

理解以下觀念：

-   ELF 檔案結構
-   .text / .data / .bss 差異
-   為什麼 .bss 不佔檔案空間
-   Demand Paging 機制
-   Page Fault 如何觸發
-   Page 與 Cache Line 的差異
-   malloc 大小與 mmap / brk 的關係
-   RSS 成長與 munmap 的影響

------------------------------------------------------------------------

## 二、.bss 是什麼？

.bss (Block Started by Symbol) 用來存放：

-   未初始化的 global 變數
-   未初始化的 static 變數

範例：

``` c
int a;          // 進入 .bss
static int b;   // 進入 .bss
```

與 .data 的差別：

``` c
int x = 5;  // .data
int y;      // .bss
```

------------------------------------------------------------------------

## 三、為什麼 .bss 不佔檔案空間？

因為未初始化變數預設為 0。

若將大量 0 存入檔案會浪費空間，因此 ELF 設計為：

-   .bss 在檔案中標記為 NOBITS
-   僅記錄大小
-   執行時由 loader 建立對應記憶體區域

在 readelf -S 中會看到：

    Type: NOBITS

------------------------------------------------------------------------

## 四、.bss 如何在執行時出現？

當 execve 發生：

1.  Kernel 讀取 ELF program header
2.  建立 LOAD segment
3.  建立對應 VMA (Virtual Memory Area)
4.  超過檔案大小但屬於 segment 的部分 由 kernel 分配 page 並初始化為 0

關鍵點：

Linux 保證新分配的 page 內容為 0（安全性需求）。

------------------------------------------------------------------------

## 五、Demand Paging 機制

若宣告：

``` c
char big[100 * 1024 * 1024];
```

發生流程：

-   建立 100MB 虛擬位址空間
-   尚未分配實體記憶體
-   當第一次存取某 page 時發生 page fault
-   Kernel 分配 4KB page
-   更新 page table
-   RSS 增加 4KB

重要：

最小分配單位是「page」，通常 4KB\
不是 cache line（通常 64 bytes）

------------------------------------------------------------------------

## 六、Page vs Cache Line

  層級             單位
  ---------------- ------------------
  Virtual Memory   Page (4KB)
  CPU Cache        Cache Line (64B)

Page Fault 發生在 page 層級。\
Cache 行為發生在 page 已存在之後。

------------------------------------------------------------------------

## 七、malloc(100MB) 與 .bss 差異

兩者表面行為類似（皆使用 demand paging），但來源不同。

### .bss

-   來自 ELF LOAD segment
-   在 execve 時建立 VMA

### malloc(100MB)

-   大於 threshold → 使用 mmap
-   runtime 建立 VMA
-   free 會呼叫 munmap

------------------------------------------------------------------------

## 八、free 之後 RSS 會不會下降？

視 allocation 來源而定：

  分配來源               free 行為           RSS 變化
  ---------------------- ------------------- ------------
  brk (小 allocation)    不一定還給 kernel   通常不下降
  mmap (大 allocation)   munmap              會下降

free 是還給 allocator\
munmap 才是還給 kernel

------------------------------------------------------------------------

## 九、RSS 成長模型分析

### 情境 1

``` c
while (1) {
    void *p = malloc(100MB);
    free(p);
}
```

結果：

-   每輪 mmap
-   每輪 munmap
-   RSS 震盪
-   不會持續上升

### 情境 2

``` c
while (1) {
    void *p = malloc(100MB);
    memset(p, 0, 100MB);
    free(p);
}
```

結果：

-   觸發 page fault
-   RSS 成長到 100MB
-   munmap 釋放
-   RSS 下降
-   持續震盪

### 情境 3

``` c
while (1) {
    void *p = malloc(100MB);
    memset(p, 0, 100MB);
}
```

結果：

-   每輪建立新 VMA
-   每輪分配 physical pages
-   未 munmap
-   RSS 持續上升
-   最終 OOM

------------------------------------------------------------------------

## 十、今日總結

Day22 理解了：

-   ELF 如何描述記憶體佈局
-   .bss 為何不佔檔案空間
-   Demand Paging 如何運作
-   Page fault 何時發生
-   VMA 與 RSS 的差異
-   brk 與 mmap 的分配策略
-   munmap 對 RSS 的影響

這些概念構成 Linux 記憶體管理的核心基礎。

------------------------------------------------------------------------

結語：

程式的執行不是「C 語言在做事」，\
而是：

ELF 描述 → Loader 建立 VMA → Kernel 分頁管理 → CPU 存取觸發 page fault。

這是 Day22 的真正核心。
