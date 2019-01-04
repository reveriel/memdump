#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
    for (int i = 0; i < 100; i ++) {
        fprintf(stderr, "test native app is running\n");
        fprintf(stdout, "test native app is running\n");
        printf("test native app is running\n");
        printf("test native app is running\n");
        sleep(1);
    }
    return 0;
}