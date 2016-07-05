#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "remote.h"

int con_connect(struct remote *remote)
{

    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof (struct addrinfo));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(remote->hostname, remote->port, &hints, &servinfo) != 0)
        return -1;

    remote->sock = socket(servinfo->ai_family, servinfo->ai_socktype, 0);

    if (remote->sock < 0)
        return -1;

    if (connect(remote->sock, servinfo->ai_addr, servinfo->ai_addrlen) < 0)
        return -1;

    return remote->sock;

}

int con_disconnect(struct remote *remote)
{

    return close(remote->sock);

}

