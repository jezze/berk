#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include "config.h"

static int runhook(char *path, char *command)
{

    if (access(path, F_OK | X_OK) == 0)
        return system(command);

    return EXIT_FAILURE;

}

int event_begin(char *id)
{

    char path[128];
    char command[BUFSIZ];

    config_get_subpath(path, BUFSIZ, CONFIG_HOOKS, "begin");
    snprintf(command, BUFSIZ, "%s %s", path, id);

    return runhook(path, command);

}

int event_end(char *id)
{

    char path[128];
    char command[BUFSIZ];

    config_get_subpath(path, BUFSIZ, CONFIG_HOOKS, "end");
    snprintf(command, BUFSIZ, "%s %s", path, id);

    return runhook(path, command);

}

int event_start(char *remote, unsigned int run)
{

    char path[128];
    char command[BUFSIZ];

    config_get_subpath(path, BUFSIZ, CONFIG_HOOKS, "start");
    snprintf(command, BUFSIZ, "%s %s %u", path, remote, run);

    return runhook(path, command);

}

int event_stop(char *remote, unsigned int run)
{

    char path[128];
    char command[BUFSIZ];

    config_get_subpath(path, BUFSIZ, CONFIG_HOOKS, "stop");
    snprintf(command, BUFSIZ, "%s %s %u", path, remote, run);

    return runhook(path, command);

}

int event_send(char *remote)
{

    char path[BUFSIZ];
    char command[BUFSIZ];

    config_get_subpath(path, BUFSIZ, CONFIG_HOOKS, "send");
    snprintf(command, BUFSIZ, "%s", path);

    return runhook(path, command);

}

