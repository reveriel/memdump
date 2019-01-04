
#ifndef _OPT_H
#define _OPT_H

/**
 * if net == true.
 *   ip_addr:port is used;
 */
struct Option {
    int pid;
    bool net;
    char *ip_addr;
    int port;
};


#endif