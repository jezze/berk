#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libssh2.h>
#include "config.h"
#include "remote.h"

static int runhook(char *name)
{

    char path[BUFSIZ];

    if (config_gethookpath(path, BUFSIZ, name))
        return -1;

    if (access(path, F_OK | X_OK) == 0)
        return system(path);

    return 0;

}

int event_begin(char *id)
{

    printf("event=begin id=%s\n", id);

    return runhook("begin");

}

int event_end(unsigned int total, unsigned int complete, unsigned int success)
{

    printf("event=end total=%d complete=%d success=%d\n", total, complete, success);

    return runhook("end");

}

int event_start(struct remote *remote)
{

    printf("event=start name=%s pid=%d\n", remote->name, remote->pid);

    return runhook("start");

}

int event_stop(struct remote *remote, int status)
{

    printf("event=stop name=%s pid=%d status=%d\n", remote->name, remote->pid, status);

    return runhook("stop");

}

