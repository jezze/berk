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

int config_get_path(char *path, unsigned int length, char *name)
{

    return snprintf(path, length, "%s/%s", root, name) < 0;

}

int config_get_remotepath(char *path, unsigned int length, char *filename)
{

    return snprintf(path, length, "%s/%s/%s", root, CONFIG_REMOTES, filename) < 0;

}

int config_get_shortrun(char *path, unsigned int length, char *id)
{

    return snprintf(path, length, "%s/%s/%c%c", root, CONFIG_LOGS, id[0], id[1]) < 0;

}

int config_get_fullrun(char *path, unsigned int length, char *id)
{

    return snprintf(path, length, "%s/%s/%c%c/%s", root, CONFIG_LOGS, id[0], id[1], id) < 0;

}

int config_get_logdir(char *path, unsigned int length, char *id, int run)
{

    return snprintf(path, length, "%s/%s/%c%c/%s/%d", root, CONFIG_LOGS, id[0], id[1], id, run) < 0;

}

int config_get_logv(char *path, unsigned int length, char *id, int run, char *name)
{

    return snprintf(path, length, "%s/%s/%c%c/%s/%d/%s", root, CONFIG_LOGS, id[0], id[1], id, run, name) < 0;

}

int config_get_logs(char *path, unsigned int length, char *id, char *run, char *name)
{

    return snprintf(path, length, "%s/%s/%c%c/%s/%s/%s", root, CONFIG_LOGS, id[0], id[1], id, run, name) < 0;

}

int config_get_hookpath(char *path, unsigned int length, char *filename)
{

    return snprintf(path, length, "%s/%s/%s", root, CONFIG_HOOKS, filename) < 0;

}

