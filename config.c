#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include "config.h"
#include "error.h"

static char root[BUFSIZ];

void config_init()
{

    char path[BUFSIZ];

    getcwd(path, BUFSIZ);

    while (1)
    {    

        if (strlen(path) == 1)
            snprintf(root, BUFSIZ, "/%s", CONFIG_ROOT);
        else
            snprintf(root, BUFSIZ, "%s/%s", path, CONFIG_ROOT);

        if (access(root, F_OK) == 0)
            return;

        if (strlen(path) == 1)
            break;

        dirname(path);

    }

    error(ERROR_PANIC, "Could not find '%s' directory.", CONFIG_ROOT);

}

int config_getpath(char *path, unsigned int length, char *name)
{

    return snprintf(path, length, "%s/%s", root, name) < 0;

}

int config_getremotepath(char *path, unsigned int length, char *filename)
{

    return snprintf(path, length, "%s/%s/%s", root, CONFIG_REMOTES, filename) < 0;

}

int config_getlogpathbypid(char *path, unsigned int length, int pid)
{

    return snprintf(path, length, "%s/%s/%d", root, CONFIG_LOGS, pid) < 0;

}

int config_getlogpathbyname(char *path, unsigned int length, char *filename)
{

    return snprintf(path, length, "%s/%s/%s", root, CONFIG_LOGS, filename) < 0;

}

int config_gethookpath(char *path, unsigned int length, char *filename)
{

    return snprintf(path, length, "%s/%s/%s", root, CONFIG_HOOKS, filename) < 0;

}

