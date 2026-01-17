#include <stdio.h>

int main(int argc, char *argv[], char *envp[]) {
    printf("argc = %d\n", argc);

    for (int i = 0; argv[i] != NULL; i++) {
        printf("argv[%d] = %p -> %s\n", i, argv[i], argv[i]);
    }

    printf("\n--- envp ---\n");
    for (int i = 0; envp[i] != NULL && i < 5; i++) {
        printf("envp[%d] = %p -> %s\n", i, envp[i], envp[i]);
    }

    return 0;
}