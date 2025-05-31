#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libssh2.h>
#include "config.h"
#include "log.h"
#include "remote.h"

static int runhook(char *name)
{

    char path[BUFSIZ];

    if (config_get_subpath(path, BUFSIZ, CONFIG_HOOKS, name))
        return -1;

    if (access(path, F_OK | X_OK) == 0)
        return system(path);

    return 0;

}

int event_begin(struct log_entry *entry)
{

    printf("event=begin id=%s\n", entry->id);

    return runhook("begin");

}

int event_end(struct log_entry *entry)
{

    printf("event=end total=%d complete=%d success=%d\n", entry->total, entry->complete, entry->success);

    return runhook("end");

}

int event_start(struct remote *remote)
{

    printf("event=start name=%s run=%d\n", remote->name, remote->run.index);

    return runhook("start");

}

int event_stop(struct remote *remote, int status)
{

    printf("event=stop name=%s run=%d status=%d\n", remote->name, remote->run.index, status);

    return runhook("stop");

}

