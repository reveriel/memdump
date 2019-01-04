#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
// #include <sys/types.h>
#include "sock.h"
#include "opt.h"

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

void dump_memory_region_send(FILE* pMemFile, unsigned long start_address, long length, int serverSocket)
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

