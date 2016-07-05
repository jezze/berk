#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "error.h"
#include "remote.h"

static int gethookpath(char *path, unsigned int length, char *filename)
{

    return snprintf(path, length, "%s/%s", CONFIG_HOOKS, filename) < 0;

}

static void runhook(char *name)
{

    char path[BUFSIZ];

    if (gethookpath(path, BUFSIZ, name))
        return;

    if (access(path, F_OK | X_OK) == 0)
        system(path);

}

void event_begin(unsigned int total)
{

    fprintf(stdout, "event=begin total=%d\n", total);
    runhook("begin");

}

void event_end(unsigned int total, unsigned int complete, unsigned int success)
{

    fprintf(stdout, "event=end total=%d complete=%d success=%d\n", total, complete, success);
    runhook("end");

}

void event_start(struct remote *remote)
{

    fprintf(stdout, "event=start name=%s pid=%d\n", remote->name, remote->pid);
    runhook("start");

}

void event_stop(struct remote *remote, int status)
{

    fprintf(stdout, "event=stop name=%s pid=%d status=%d\n", remote->name, remote->pid, status);
    runhook("stop");

}

