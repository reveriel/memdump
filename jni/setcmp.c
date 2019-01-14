/**
 * setcmp, compare a set of process. calculate redundancy.
 *
 *   for each <pid>, compute how many pages of it are not unique.
 *
 * usage:
 *    setcmp <pid> <pid> <pid> <pid> ... <pid>
 */
#include <stdio.h>
#include <stdlib.h>
#include "mem.h"
#include "simi.h"

#define MAX_PID 32

void set_add_mr(struct Set *s, struct MemReg *mr)
{
    for (int i = 0; i < mr_page_num(mr); i++) {
        struct Page *page = mr_get_page(mr, i);
        if (page_is_zero(page))
            continue;
        set_add(s, data_init(page_to_u32(page)));
    }
}

struct Set *set_from_proc(struct Process *p)
{
    struct Set *s = set_init();

    for (int i = 0; i < proc_mr_num(p); i++)
    {
        struct MemReg *mr = proc_get_mr(p, i);
        set_add_mr(s, mr);
    }
    return s;
}

// the set created will have no duplicated elements
struct Set *uniq_set_from_proc(struct Process *p)
{
    struct Set *s = set_init();
    for (int i = 0; i < proc_mr_num(p); i++)
    {
        struct MemReg *mr = proc_get_mr(p, i);
        for (int j = 0; j < mr_page_num(mr); j++)
        {
            struct Page *page = mr_get_page(mr, j);
            if (page_is_zero(page))
                continue;
            struct Data *d = data_init(page_to_u32(page));
            if (set_in(s, d))
            {
                data_free(d);
                continue;
            }
            set_add(s, d);
        }
    }
    return s;
}

void print_intra_simi(struct Process **proc, int num)
{
    for (int i = 0; i < num; i++) {
        struct Set *set = set_from_proc(proc[i]);
        struct Set *uniq_set = uniq_set_from_proc(proc[i]);

        fprintf(stderr, "%d %d\n", set_size(set) - set_size(uniq_set), set_size(set));

        set_free(set);
        set_free(uniq_set);
    }
}

// how many nonzero pages can be found in other proceses.
// this is inter process
void print_inter_simi(struct Process **proc, int num)
{
    struct Set *set[MAX_PID];

    for (int i = 0; i < num; i++)
        set[i] = set_from_proc(proc[i]);

    // inter process
    for (int i = 0; i < num; i++)
    {

        int dup_page_nr = 0;
        struct Data *d = set_first(set[i]);
        do {
            int found = 0;

            for (int j = 0; j < num; j++) {
                if (j == i)
                    continue;
                if (set_in(set[j], d)) {
                    found = 1;
                    break;
                }
            }

            dup_page_nr += found;
        } while (d = set_next(d), d);

        // fprintf(stderr, "pid = %d, dup: %d %d\n", proc_get_pid(proc[i]),
                // dup_page_nr, set_size(set[i]));

        fprintf(stderr, "%d %d\n", dup_page_nr, set_size(set[i]));
    }

    for (int i = 0; i < num; i++)
        set_free(set[i]);
}

int main(int argc, char const *argv[])
{
    if (argc <= 2 || argc >= MAX_PID)
    {
        fprintf(stderr, "%s <pid> <pid> ...  <pid>\n", argv[0]);
        return -1;
    }

    int num_pid = argc - 1;
    int pid[MAX_PID];
    struct Process *proc[MAX_PID];
    for (int i = 0; i < num_pid; i++) {
        pid[i] = atoi(argv[i+1]);
        proc[i] = proc_init(pid[i]);
    }

    for (int i = 0; i < num_pid; i++) {
        // fprintf(stderr, "%d\n", pid[i]);
        proc_attach(proc[i]);
        proc_do(proc[i]);
        proc_detach(proc[i]);
    }


    // print_inter_simi(proc, num_pid);

    print_intra_simi(proc, num_pid);



    for (int i = 0; i < num_pid; i++) {
        proc_del(proc[i]);
    }
    return 0;
}


