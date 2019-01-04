#ifndef _MEM_H
#define _MEM_H

struct Option;

struct Process;

struct MemReg;

struct Page;


struct Process *proc_init(struct Option *opt);
void proc_del(struct Process *p);
void proc_do(struct Process *p);
void proc_print_maps(struct Process *p);
// attach to 'p', also wait until 'p' stops.
int proc_attach(struct Process *p);
void proc_detach(struct Process *p);

#endif