/** memdup
 * dup a process's pages
 *
 * usage:
 * memdump <pid>
 */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#include "mem.h"
#include "opt.h"

struct Option opt;

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        opt.net = false;
        opt.pid = atoi(argv[1]);
    }
    else if (argc == 4)
    {
        opt.net = true;
        opt.pid = atoi(argv[1]);
        opt.ip_addr = argv[2];
        int count = sscanf(argv[3], "%d", &opt.port);
        if (count == 0)
        {
            printf("Invalid port specified\n");
            return -1;
        }
    }
    else
    {
        printf("%s <pid>\n", argv[0]);
        printf("%s <pid> <ip-address> <port>\n", argv[0]);
        exit(0);
    }

    struct Process *p = proc_init(opt.pid);

    proc_attach(p);
    proc_do(p);
    proc_detach(p);

    // proc_print_maps(p);
    // proc_print_pages(p);

    // similarity of each pair of memory region
    // given two process ...
    // I'd use another programm to do that.

    proc_del(p);

    return 0;
}
