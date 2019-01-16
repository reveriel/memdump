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

// add all non-zero pages in 'mr' to set 's'
void set_add_mr(struct Set *s, struct MemReg *mr)
{
    for (int i = 0; i < mr_page_num(mr); i++)
    {
        struct Page *page = mr_get_page(mr, i);
        if (page_is_zero(page))
            continue;
        set_add(s, data_init(page_to_u32(page)));
    }
}

// build a set using all nonzero pages from 'mr'
struct Set *set_from_mr(struct MemReg *mr)
{
    struct Set *s = set_init();
    set_add_mr(s, mr);
    return s;
}

struct Set *uniq_set_from_mr(struct MemReg *mr)
{
    struct Set *s = set_init();
    for (int i = 0; i < mr_page_num(mr); i++)
    {
        struct Page *page = mr_get_page(mr, i);
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
    return s;
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
    fprintf(stderr, "intra proc dup\n");
    for (int i = 0; i < num; i++)
    {
        struct Set *set = set_from_proc(proc[i]);
        struct Set *uniq_set = uniq_set_from_proc(proc[i]);

        fprintf(stderr, "%d\n", set_size(set) - set_size(uniq_set));

        set_free(set);
        set_free(uniq_set);
    }
}

// if found, return 1;
// else return 0;
int found_in_other_sets(struct Set *set[], int num, struct Data *d, int this)
{
    int found = 0;
    for (int i = 0; i < num; i++)
    {
        if (i == this)
            continue;
        if (set_in(set[i], d))
        {
            found = 1;
            break;
        }
    }
    return found;
}

// how many nonzero pages can be found in other proceses.
// this is inter process
void print_inter_simi(struct Process **proc, int num)
{
    fprintf(stderr, "inter proc dup\n");
    struct Set *set[MAX_PID];

    for (int i = 0; i < num; i++)
        set[i] = set_from_proc(proc[i]);

    // inter process
    for (int i = 0; i < num; i++)
    {
        int dup_page_nr = 0;
        struct Data *d = set_first(set[i]);
        do
        {
            dup_page_nr += found_in_other_sets(set, num, d, i);
        } while (d = set_next(d), d);

        // fprintf(stderr, "pid = %d, dup: %d %d\n", proc_get_pid(proc[i]),
        // dup_page_nr, set_size(set[i]));

        fprintf(stderr, "%d \n", dup_page_nr);
    }

    for (int i = 0; i < num; i++)
        set_free(set[i]);
}

void print_simi_matrix(struct Process **proc, int num)
{
    fprintf(stderr, "simi matrix\n");
    struct Set *set[MAX_PID];
    for (int i = 0; i < num; i++)
        set[i] = set_from_proc(proc[i]);

    for (int i = 0; i < num; i++)
    {
        for (int j = 0; j < num; j++)
        {
            if (i == j)
            {
                fprintf(stderr, "0 ");
                continue;
            }

            fprintf(stderr, "%d ", set_found_in(set[i], set[j]));
        }

        fprintf(stderr, "\n");
    }

    for (int i = 0; i < num; i++)
        set_free(set[i]);
}

// return mem region's dup number
// i.e. how many pages in 'mr' can be found in other processes
int mr_dup_inter(struct Set *set[], int num, struct MemReg *mr, int this)
{
    int dup = 0;
    for (int i = 0; i < mr_page_num(mr); i++)
    {
        struct Page *page = mr_get_page(mr, i);
        if (page_is_zero(page))
            continue;
        struct Data *d = data_init(page_to_u32(page));
        dup += found_in_other_sets(set, num, d, this);
        data_free(d);
    }
    return dup;
}

// return number of nonzero pages in 'mr'
int mr_nonzero_num(struct MemReg *mr)
{
    int n = 0;
    for (int i = 0; i < mr_page_num(mr); i++)
        if (!page_is_zero(mr_get_page(mr, i)))
            n++;
    return n;
}

// found which mr has more dup
void print_inter_simi_mr(struct Process **proc, int num)
{
    struct Set *set[MAX_PID];

    for (int i = 0; i < num; i++)
        set[i] = set_from_proc(proc[i]);

    // for each process
    for (int i = 0; i < num; i++)
    {
        // for each mem region
        for (int j = 0; j < proc_mr_num(proc[i]); j++)
        {
            struct MemReg *mr = proc_get_mr(proc[i], j);
            int dup = mr_dup_inter(set, num, mr, i);

            if (dup == 0)
                continue;

            fprintf(stderr, "%d %d %s\n",
                    dup, mr_nonzero_num(mr), mr_get_name(mr));
        }
    }
}
void print_total_dup(struct Process **proc, int num)
{
    fprintf(stderr, "total dup\n");

    struct Set *set[MAX_PID];
    struct Set *uniq_set[MAX_PID];
    for (int i = 0; i < num; i++)
    {
        set[i] = set_from_proc(proc[i]);
        uniq_set[i] = uniq_set_from_proc(proc[i]);
    }

    for (int i = 0; i < num; i++)
    {
        int dup_page_nr = 0;
        struct Data *d = set_first(uniq_set[i]);
        do
        {
            dup_page_nr += found_in_other_sets(uniq_set, num, d, i);
        } while (d = set_next(d), d);

        fprintf(stderr, "%d %d \n",
                dup_page_nr + set_size(set[i]) - set_size(uniq_set[i]),
                set_size(set[i]));
    }
}

int intra_mr_simi(struct MemReg *mr)
{
    struct Set *set = set_from_mr(mr);
    struct Set *uniq_set = uniq_set_from_mr(mr);

    int dup =  set_size(set) - set_size(uniq_set);

    set_free(set);
    set_free(uniq_set);
    return dup;
}

static void intra_proc_simi_mr(struct Process *p)
{
    int n_mr = proc_mr_num(p);
    struct Set **set = (struct Set **)malloc(sizeof(struct Set *) * n_mr);
    // create a set for each mr
    for (int i = 0; i < n_mr; i++)
        set[i] = set_from_mr(proc_get_mr(p, i));

    // // for each mr
    for (int i = 0; i < n_mr; i++)
    {
        // for each page in set[i],
        int dup_page_nr = 0;
        struct MemReg *mr = proc_get_mr(p, i);

        // =-=================== buggy, why ?
        // struct Data *d = set_first(set[i]);
        // do
        // {
        //     dup_page_nr += found_in_other_sets(set, n_mr, d, i);
        // } while (d = set_next(d), d);
        // ========================

        for (int j = 0; j < mr_page_num(mr); j++)
        {
            struct Page *page = mr_get_page(mr, j);
            struct Data *d = data_init(page_to_u32(page));
            // if  pages in an vma are the same
            // can not found in this way !
            dup_page_nr += found_in_other_sets(set, n_mr, d, i);
            data_free(d);
        }

        if (dup_page_nr == 0)
            continue;

        fprintf(stderr, "%d %d %s\n",
                intra_mr_simi(mr), // dup found in same mr
                dup_page_nr,  // dup found in other mr of same proc
                mr_get_name(mr));
    }

    for (int i = 0; i < n_mr; i++)
        set_free(set[i]);
    free(set);
}


// mr duplication, intra process
// in different vma;
void print_intra_process_simi_mr(struct Process **proc, int num)
{
    for (int i = 0; i < num; i++)
        intra_proc_simi_mr(proc[i]);
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
    for (int i = 0; i < num_pid; i++)
    {
        pid[i] = atoi(argv[i + 1]);
        proc[i] = proc_init(pid[i]);
    }

    for (int i = 0; i < num_pid; i++)
    {
        // fprintf(stderr, "%d\n", pid[i]);
        proc_attach(proc[i]);
        proc_do(proc[i]);
        proc_detach(proc[i]);
    }

    // print_intra_simi(proc, num_pid); fprintf(stderr, "\n");
    // print_inter_simi(proc, num_pid); fprintf(stderr, "\n");
    // print_total_dup(proc, num_pid); fprintf(stderr, "\n");

    // print_simi_matrix(proc, num_pid); fprintf(stderr, "\n");

    // print_inter_simi_mr(proc, num_pid); fprintf(stderr, "\n");

    print_intra_process_simi_mr(proc, num_pid);


    for (int i = 0; i < num_pid; i++)
    {
        proc_del(proc[i]);
    }
    return 0;
}
