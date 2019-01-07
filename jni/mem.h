#ifndef _MEM_H
#define _MEM_H
#include <stdint.h>

struct Process;

struct MemReg;

struct Page;


struct Process *proc_init(int pid);
void proc_del(struct Process *p);
void proc_do(struct Process *p);
void proc_print_maps(struct Process *p);
void proc_print_pages(struct Process *p);
// attach to 'p', also wait until 'p' stops.
int proc_attach(struct Process *p);
void proc_detach(struct Process *p);


// return number of memory regions
int proc_mr_num(struct Process *p);
struct MemReg *proc_get_mr(struct Process *p, int index);
// return number of pages in mr
int mr_page_num(struct MemReg *);
struct Page *mr_get_page(struct MemReg *m, int index);
const char *mr_get_name(struct MemReg *m);

uint32_t page_to_u32(struct Page *p);
int page_is_zero(struct Page *p);

#endif
