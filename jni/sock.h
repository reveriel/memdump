#ifndef _SOCK_H
#define _SOCK_H

struct Option;
struct FILE;

struct Sock
{
    int fd;
};

int open_socket(struct Sock *sock, struct Option *opt);
void dump_memory_region_send(FILE *pMemFile, unsigned long start_address, long length, int serverSocket);

#endif