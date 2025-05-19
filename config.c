#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include "config.h"

static char root[BUFSIZ + 32];

int config_init()
{

    char path[BUFSIZ];

    getcwd(path, BUFSIZ);

    while (1)
    {    

        if (strlen(path) == 1)
            snprintf(root, BUFSIZ + 32, "/%s", CONFIG_ROOT);
        else
            snprintf(root, BUFSIZ + 32, "%s/%s", path, CONFIG_ROOT);

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

int config_getshortrun(char *path, unsigned int length, char *id)
{

    return snprintf(path, length, "%s/%s/%c%c", root, CONFIG_LOGS, id[0], id[1]) < 0;

}

int config_getfullrun(char *path, unsigned int length, char *id)
{

    return snprintf(path, length, "%s/%s/%c%c/%s", root, CONFIG_LOGS, id[0], id[1], id) < 0;

}

int config_getlogdir(char *path, unsigned int length, char *id, int pid)
{

    return snprintf(path, length, "%s/%s/%c%c/%s/%d", root, CONFIG_LOGS, id[0], id[1], id, pid) < 0;

}

int config_getlogvstderr(char *path, unsigned int length, char *id, int pid)
{

    return snprintf(path, length, "%s/%s/%c%c/%s/%d/stderr", root, CONFIG_LOGS, id[0], id[1], id, pid) < 0;

}

int config_getlogsstderr(char *path, unsigned int length, char *id, char *pid)
{

    return snprintf(path, length, "%s/%s/%c%c/%s/%s/stderr", root, CONFIG_LOGS, id[0], id[1], id, pid) < 0;

}

int config_getlogvstdout(char *path, unsigned int length, char *id, int pid)
{

    return snprintf(path, length, "%s/%s/%c%c/%s/%d/stdout", root, CONFIG_LOGS, id[0], id[1], id, pid) < 0;

}

int config_getlogsstdout(char *path, unsigned int length, char *id, char *pid)
{

    return snprintf(path, length, "%s/%s/%c%c/%s/%s/stdout", root, CONFIG_LOGS, id[0], id[1], id, pid) < 0;

}

int config_gethookpath(char *path, unsigned int length, char *filename)
{

    return snprintf(path, length, "%s/%s/%s", root, CONFIG_HOOKS, filename) < 0;

}

