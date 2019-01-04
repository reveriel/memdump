#ifndef _MEM_H
#define _MEM_H

struct Option;

struct Process;

struct MemReg;

struct Page;


struct Process *proc_init(struct Option *opt);
void proc_del(struct Process *p);
void proc_do(struct Process *p);


#endif