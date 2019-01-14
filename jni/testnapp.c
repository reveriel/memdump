#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// print to stderr, or you cannot see it with adb shell

#define BUF_SIZE (4096 * 4)
char g_buf[BUF_SIZE];

static void do_something() {
    for (int i = 0; i < BUF_SIZE; i++)
        g_buf[i] = 1;

    char *buf = (char *)malloc(BUF_SIZE);
    for (int i = 0; i < BUF_SIZE; i++)
        buf[i] = 2;
}

int main()
{

    do_something();

    for (int i = 0; i < 100; i++)
    {
        fprintf(stderr, "pid = %d, test native app is running\n", getpid());
        sleep(1);
    }
    return 0;
}