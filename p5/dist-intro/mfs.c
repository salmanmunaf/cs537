#include <string.h>
#include "udp.h"
#include "mfs.h"

int sendMessage(MFS_Message_t *req, MFS_Message_t *res, char *hostname, int port)
{
    int sd = UDP_Open(0);
    if (sd < -1)
    {
        printf("sendMessage: failed to open socket");
        return -1;
    }

    struct sockaddr_in addr, addr2;
    int rc = UDP_FillSockAddr(&addr, hostname, port);
    if (rc < 0)
    {
        printf("sendMessage: failed to find host");
        return -1;
    }

    fd_set rfds;
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    while (1)
    {
        FD_ZERO(&rfds);
        FD_SET(sd, &rfds);
        UDP_Write(sd, &addr, (char *)req, sizeof(MFS_Message_t));
        if (select(sd + 1, &rfds, NULL, NULL, &tv))
        {
            rc = UDP_Read(sd, &addr2, (char *)res, sizeof(MFS_Message_t));
            if (rc > 0)
            {
                UDP_Close(sd);
                return 0;
            }
        }
    }
}

char *server_hostname;
int server_port;
int initialized = 0;

int MFS_Init(char *hostname, int port)
{
    if (port < 0 || strlen(hostname) < 1)
        return -1;
    server_hostname = malloc(strlen(hostname) + 1);
    strcpy(server_hostname, hostname);
    server_port = port;
    initialized = 1;
    return 0;
}

int MFS_Lookup(int pinum, char *name)
{
    if (!initialized)
        return -1;

    if (pinum < 0 || pinum >= TOTAL_NUM_INODES)
        return -1;

    if (strlen(name) > LEN_NAME || name == NULL)
        return -1;

    MFS_Message_t req;
    MFS_Message_t res;

    req.inum = pinum;
    req.request = REQ_LOOKUP;

    strcpy((char *)&(req.name), name);

    if (sendMessage(&req, &res, server_hostname, server_port) < 0)
        return -1;
    else
        return res.inum;
}

int MFS_Stat(int inum, MFS_Stat_t *m)
{
    if (!initialized)
        return -1;

    if (inum < 0 || inum >= TOTAL_NUM_INODES)
        return -1;

    MFS_Message_t req;
    req.inum = inum;
    req.request = REQ_STAT;

    MFS_Message_t res;
    if (sendMessage(&req, &res, server_hostname, server_port) < 0)
        return -1;
    m->type = res.stat.type;
    m->size = res.stat.size;

    return 0;
}

int MFS_Write(int inum, char *buffer, int block)
{
    if (!initialized)
        return -1;

    if (inum < 0 || inum >= TOTAL_NUM_INODES)
        return -1;

    if (block < 0 || block > NUM_INODE_POINTERS - 1)
        return -1;

    MFS_Message_t req;
    MFS_Message_t res;

    req.inum = inum;

    for (int i = 0; i < MFS_BLOCK_SIZE; i++)
        req.buffer[i] = buffer[i];

    req.block = block;
    req.request = REQ_WRITE;

    if (sendMessage(&req, &res, server_hostname, server_port) < 0)
        return -1;

    return res.inum;
}

int MFS_Read(int inum, char *buffer, int block)
{
    if (!initialized)
        return -1;

    if (inum < 0 || inum >= TOTAL_NUM_INODES)
        return -1;

    if (block < 0 || block > NUM_INODE_POINTERS - 1)
        return -1;

    MFS_Message_t req;
    req.inum = inum;
    req.block = block;
    req.request = REQ_READ;

    MFS_Message_t res;
    if (sendMessage(&req, &res, server_hostname, server_port) < 0)
        return -1;

    if (res.inum > -1)
    {
        for (int i = 0; i < MFS_BLOCK_SIZE; i++)
            buffer[i] = res.buffer[i];
    }

    return res.inum;
}

int MFS_Creat(int pinum, int type, char *name)
{
    if (!initialized)
        return -1;

    if (pinum < 0 || pinum >= TOTAL_NUM_INODES)
        return -1;

    if (strlen(name) > LEN_NAME || name == NULL)
        return -1;

    MFS_Message_t req;

    strcpy(req.name, name);
    req.inum = pinum;
    req.type = type;
    req.request = REQ_CREAT;

    MFS_Message_t res;
    if (sendMessage(&req, &res, server_hostname, server_port) < 0)
        return -1;

    return res.inum;
}

int MFS_Unlink(int pinum, char *name)
{
    if (!initialized)
        return -1;

    if (pinum < 0 || pinum >= TOTAL_NUM_INODES)
        return -1;

    if (strlen(name) > LEN_NAME || name == NULL)
        return -1;

    MFS_Message_t req;
    req.inum = pinum;
    req.request = REQ_UNLINK;
    strcpy(req.name, name);

    MFS_Message_t res;
    if (sendMessage(&req, &res, server_hostname, server_port) < 0)
        return -1;

    return res.inum;
}

int MFS_Shutdown()
{
    MFS_Message_t req;
    req.request = REQ_SHUTDOWN;

    MFS_Message_t res;
    if (sendMessage(&req, &res, server_hostname, server_port) < 0)
        return -1;

    return 0;
}