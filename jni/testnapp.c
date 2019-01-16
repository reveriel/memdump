#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>

// print to stderr, or you cannot see it with adb shell

#define BUF_SIZE (4096 * 100)
char g_buf[BUF_SIZE];

static void do_something(char *l_buf) {
    memset(g_buf, '1',  BUF_SIZE);

    char *buf = (char *)malloc(BUF_SIZE * 2);
    memset(buf, '2', BUF_SIZE * 2);

    memset(l_buf, '3', BUF_SIZE * 3);

    char *m_buf = mmap(0, BUF_SIZE * 4, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    memset(m_buf, '4', BUF_SIZE * 4);
}

int main()
{

    char l_buf[BUF_SIZE * 3];

    do_something(l_buf);

    for (int i = 0; i < 100; i++)
    {
        fprintf(stderr, "pid = %d, test native app is running\n", getpid());
        sleep(1);
    }
    return 0;
}