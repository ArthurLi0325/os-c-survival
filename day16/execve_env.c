#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(void) {
    printf("[before execve] getenv(PATH)=%s\n", getenv("PATH"));

    char *argv[] = { "show_env", NULL };

    // 注意：這裡故意只帶兩個自訂環境變數，不帶 PATH
    char *envp[] = {
        "DAY16_A=hello",
        "DAY16_B=world",
        NULL
    };

    execve("./day16/show_env", argv, envp);

    perror("execve failed");
    return 1;
}
