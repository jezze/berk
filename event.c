#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libssh2.h>
#include "config.h"
#include "log.h"
#include "run.h"
#include "remote.h"

static int runhook(char *name)
{

    char path[BUFSIZ];

    config_get_subpath(path, BUFSIZ, CONFIG_HOOKS, name);

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

    printf("event=end total=%d complete=%d passed=%d failed=%d\n", entry->total, entry->complete, entry->passed, entry->failed);

    return runhook("end");

}

int event_start(struct remote *remote, struct run *run)
{

    printf("event=start remote=%s run=%d\n", remote->name, run->index);

    return runhook("start");

}

int event_stop(struct remote *remote, struct run *run)
{

    printf("event=stop remote=%s run=%d\n", remote->name, run->index);

    return runhook("stop");

}

int event_send(struct remote *remote)
{

    printf("event=send remote=%s\n", remote->name);

    return runhook("send");

}

