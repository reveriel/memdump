#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// print to stderr, or you cannot see it with adb shell

int main() {
    for (int i = 0; i < 100; i ++) {
        fprintf(stderr, "pid = %d, test native app is running\n", getpid());
        sleep(1);
    }
    return 0;
}