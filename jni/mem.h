#ifndef _MEM_H
#define _MEM_H
#include <stdint.h>
#include <stdbool.h>

struct Process;

struct MemReg;

struct Page;

struct Process *proc_init(int pid);
void proc_del(struct Process *p);

// parse /proc/<pid>/maps
// then (may) filter anonymous pages.
// then read each page in /proc/<pid>/mem
// require proc_attach() is called.
void proc_do(struct Process *p);

// use this after calling 'proc_do()'
void proc_parse_pagemap(struct Process *p);

void proc_print_maps(struct Process *p);
void proc_print_pages(struct Process *p);
int proc_get_pid(struct Process *p);

// only parse /proc/pid/maps
void proc_do_maps(struct Process *p);

// attach to 'p', also wait until 'p' stops.
int proc_attach(struct Process *p);
// detach from 'p', process 'p' will resume.
void proc_detach(struct Process *p);

void proc_save(const char *filename);
struct Process *proc_load(const char *filename);

// return number of memory regions
int proc_mr_num(struct Process *p);
struct MemReg *proc_get_mr(struct Process *p, int index);
// return number of pages in mr
int mr_page_num(struct MemReg *);
struct Page *mr_get_page(struct MemReg *m, int index);
const char *mr_get_name(struct MemReg *m);
unsigned long mr_get_start(struct MemReg *m);
char *mr_get_perm(struct MemReg *m);
bool mr_is_anon(struct MemReg *m);


uint32_t page_to_u32(struct Page *p);
int page_is_zero(struct Page *p);


void mem_time_report();

#endif
