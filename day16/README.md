# Day16 - exec / execve：Process Image Replacement

## 今日目標
1. 理解 exec 的本質：不是建立新 process，而是「替換目前 process 的整個使用者態記憶體映像」。
2. 用 /proc/self/maps 證明 exec 前後 mapping 會完全改變。
3. 用 execve(argv, envp) 證明 argv/envp 由 kernel 建立新 stack 並交給新程式。

---

## 觀念整理：exec 到底做了什麼
- fork：複製出新的 process（透過 Copy-on-Write 延後複製）
- exec：在「同一個 PID」下，把原本程式的 text/heap/stack 等記憶體佈局整個替換成另一個可執行檔
- exec 成功後不會 return；只有失敗才會回到原程式並可用 perror 觀察原因

---

## 實驗 1：exec_basic.c（exec 成功後不會回來）
檔案：exec_basic.c

重點：
- 先印出 PID
- execl("/bin/ls", ...) 成功後，原本程式就被 /bin/ls 取代

編譯：
gcc -O0 -g -Wall -Wextra -o day16/exec_basic day16/exec_basic.c

執行：
./day16/exec_basic

---

## 實驗 2：fork_exec.c（shell 的核心模型）
檔案：fork_exec.c

重點：
- parent fork 出 child
- child exec 成另一個程式（例如 ls）
- parent wait 等 child 結束

編譯：
gcc -O0 -g -Wall -Wextra -o day16/fork_exec day16/fork_exec.c

執行：
./day16/fork_exec

---

## 實驗 3：maps_before_exec.c + show_maps.c（同 PID、不同 process image）
檔案：
- maps_before_exec.c：exec 前印 /proc/self/maps
- show_maps.c：exec 後印 /proc/self/maps 與 argv

觀察重點：
1. PID 不變（exec 前後皆印出相同 pid）
2. /proc/self/maps 內，主程式路徑從 maps_before_exec 變成 show_maps
3. argv 會是 exec 傳進去的新參數（代表新 stack）

編譯：
gcc -O0 -g -Wall -Wextra -o day16/show_maps day16/show_maps.c
gcc -O0 -g -Wall -Wextra -o day16/maps_before_exec day16/maps_before_exec.c

執行：
./day16/maps_before_exec

---

## 實驗 4：execve_env.c + show_env.c（envp 決定新程式環境）
檔案：
- execve_env.c：呼叫 execve("./day16/show_env", argv, envp)
- show_env.c：用 getenv 觀察環境變數

觀察重點：
- execve 之後，新程式的環境變數只包含 envp 指定的內容
- 若 envp 未提供 PATH，則新程式 getenv("PATH") 會是 (null)

編譯：
gcc -O0 -g -Wall -Wextra -o day16/show_env day16/show_env.c
gcc -O0 -g -Wall -Wextra -o day16/execve_env day16/execve_env.c

執行：
./day16/execve_env

---

## 編譯旗標說明
- -O0：關閉最佳化，避免程式被編譯器改寫得太難用 gdb 追
- -g：產生除錯資訊，讓 gdb 能對應回原始碼行號與變數
- -Wall：開啟常用警告，抓出多數可疑寫法
- -Wextra：補充更多額外警告（比 -Wall 更嚴格）
