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
#include <string.h>

#include "mem.h"
#include "opt.h"

struct Option opt;

// print vma number
// named vs anonymous
// pages
// size of vma

void maps_stat(int pid)
{
    struct Process *p = proc_init(pid);
    proc_do_maps(p);

    int nr_anon = 0;
    for (int i = 0; i < proc_mr_num(p); i++)
    {
        struct MemReg *mr = proc_get_mr(p, i);
        if (mr_is_anon(mr))
        {
            nr_anon++;
            // fprintf(stderr, "%d %s\n", mr_page_num(mr), mr_get_name(mr));
        }
    }
    fprintf(stderr, "%d %d\n", nr_anon, proc_mr_num(p));
}

void nonzero_stat(int pid)
{
    struct Process *p = proc_init(pid);
    proc_do(p);

    for (int i = 0; i < proc_mr_num(p); i++)
    {
        struct MemReg *mr = proc_get_mr(p, i);
        int nr_nonzero = 0;
        for (int j = 0; j < mr_page_num(mr); j++)
        {
            struct Page *p = mr_get_page(mr, j);
            if (!page_is_zero(p))
                nr_nonzero++;
        }

        fprintf(stderr, "%d %d %s\n", mr_page_num(mr), nr_nonzero, mr_get_name(mr));
    }
}

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        opt.pid = atoi(argv[1]);
    }
    else if (argc == 3 && !strcmp(argv[2], "-m"))
    {
        opt.pid = atoi(argv[1]);
        maps_stat(opt.pid);
        return 0;
    }
    else if (argc == 3 && !strcmp(argv[2], "-n"))
    {
        nonzero_stat(atoi(argv[1]));
        return 0;
    }
    else
    {
        printf("%s <pid>\n", argv[0]);
        printf("%s <pid> -m\n", argv[0]);
        printf("          print maps info\n");
        printf("%s <pid> -n\n", argv[0]);
        printf("          nonzero count\n");
        exit(0);
    }

    struct Process *p = proc_init(opt.pid);

    proc_attach(p);
    proc_do(p);
    proc_parse_pagemap(p);
    proc_detach(p);

    // proc_print_maps(p);
    // proc_print_pages(p);

    // similarity of each pair of memory region
    // given two process ...
    // I'd use another programm to do that.

    proc_del(p);


    mem_time_report();
    return 0;
}
