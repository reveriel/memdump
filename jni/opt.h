
#ifndef _OPT_H
#define _OPT_H
#include <stdbool.h>

/**
 * if net == true.
 *   ip_addr:port is used;
 */
struct Option
{
    int pid;
    bool net;
    char *ip_addr;
    int port;
};

#endif