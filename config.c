#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include "config.h"

static char root[BUFSIZ];

int config_init()
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
            return 0;

        if (strlen(path) == 1)
            break;

        dirname(path);

    }

    return -1;

}

int config_getpath(char *path, unsigned int length, char *name)
{

    return snprintf(path, length, "%s/%s", root, name) < 0;

}

int config_getremotepath(char *path, unsigned int length, char *filename)
{

    return snprintf(path, length, "%s/%s/%s", root, CONFIG_REMOTES, filename) < 0;

}

int config_getgroupbygid(char *path, unsigned int length, int gid)
{

    return snprintf(path, length, "%s/%s/%d", root, CONFIG_LOGS, gid) < 0;

}

int config_getgroupbyname(char *path, unsigned int length, char *gid)
{

    return snprintf(path, length, "%s/%s/%s", root, CONFIG_LOGS, gid) < 0;

}

int config_getprocessbypid(char *path, unsigned int length, int gid, int pid)
{

    return snprintf(path, length, "%s/%s/%d/%d", root, CONFIG_LOGS, gid, pid) < 0;

}

int config_getprocessbyname(char *path, unsigned int length, char *gid, char *pid)
{

    return snprintf(path, length, "%s/%s/%s/%s", root, CONFIG_LOGS, gid, pid) < 0;

}

int config_gethookpath(char *path, unsigned int length, char *filename)
{

    return snprintf(path, length, "%s/%s/%s", root, CONFIG_HOOKS, filename) < 0;

}

