#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "config.h"

static int runhook(char *name)
{

    char path[BUFSIZ];

    config_get_subpath(path, BUFSIZ, CONFIG_HOOKS, name);

    if (access(path, F_OK | X_OK) == 0)
        return system(path);

    return 0;

}

int event_begin(char *id)
{

    printf("event=begin id=%s\n", id);

    return runhook("begin");

}

int event_end(char *id)
{

    printf("event=end id=%s\n", id);

    return runhook("end");

}

int event_start(char *remote, unsigned int run)
{

    printf("event=start remote=%s run=%d\n", remote, run);

    return runhook("start");

}

int event_stop(char *remote, unsigned int run)
{

    printf("event=stop remote=%s run=%d\n", remote, run);

    return runhook("stop");

}

int event_send(char *remote)
{

    printf("event=send remote=%s\n", remote);

    return runhook("send");

}

