#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdbool.h>

#include "mem.h"
#include "opt.h"


void dump_memory_region(FILE* pMemFile, unsigned long start_address, long length, int serverSocket)
{
    unsigned long address;
    int pageLength = 4096;
    unsigned char page[pageLength];
    fseeko(pMemFile, start_address, SEEK_SET);

    for (address=start_address; address < start_address + length; address += pageLength)
    {
        fread(&page, 1, pageLength, pMemFile);
        if (serverSocket == -1)
        {
            // write to stdout
            fwrite(&page, 1, pageLength, stdout);
        }
        else
        {
            send(serverSocket, &page, pageLength, 0);
        }
    }
}

struct Option opt;

struct Sock {
    int fd;
} sock;

int open_socket(struct Sock *sock, struct Option *opt);

int main(int argc, char **argv)
{
    if (argc == 2) {
        opt.net = false;
        opt.pid = atoi(argv[1]);
    } else if (argc == 4) {
        opt.net = true;
        opt.pid = atoi(argv[1]);
        opt.ip_addr = argv[2];
        int count = sscanf(argv[3], "%d", &opt.port);
        if (count == 0) {
            printf("Invalid port specified\n");
            return -1;
        }
    } else {
        printf("%s <pid>\n", argv[0]);
        printf("%s <pid> <ip-address> <port>\n", argv[0]);
        exit(0);
    }

    struct Process *p = proc_init(&opt);

    if (proc_attach(p)) {
        proc_del(p);
        return -1;
    }

    proc_do(p);
    proc_print_maps(p);

    proc_del(p);

    proc_detach(p);
    return 0;
}


/// set sock->fd;
int open_socket(struct Sock *sock, struct Option *opt) {
    int serverSocket = -1;
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        fprintf(stderr, "Could not create socket\n");
        return -1;
    }
    struct sockaddr_in serverSocketAddress;
    serverSocketAddress.sin_addr.s_addr = inet_addr(opt->ip_addr);
    serverSocketAddress.sin_family = AF_INET;
    serverSocketAddress.sin_port = htons(opt->port);
    if (connect(serverSocket, (struct sockaddr *)&serverSocketAddress,
                sizeof(serverSocketAddress)) < 0)
    {
        fprintf(stderr, "Could not connect to server\n");
        return -1;
    }
    sock->fd = serverSocket;
    return 0;
}
