#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include "ini.h"
#include "config.h"

static char root[BUFSIZ];

static int loadcallback(void *user, char *section, char *key, char *value)
{

    struct config_core *core = user;

    if (strcmp(section, "core"))
        return 0;

    if (strlen(value))
    {

        if (!strcmp(key, "version"))
            core->version = strdup(value);

    }

    return 0;

}

int config_load(struct config_core *core)
{

    char path[BUFSIZ];

    config_get_path(path, BUFSIZ, CONFIG_PATH);

    return ini_parse(path, loadcallback, core);

}

int config_save(struct config_core *core)
{

    char path[BUFSIZ];
    int fd;

    config_get_path(path, BUFSIZ, CONFIG_PATH);

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (fd > 0)
    {

        ini_write_section(fd, "core");

        if (core->version && strlen(core->version))
            ini_write_string(fd, "version", core->version);

        close(fd);

        return 0;

    }

    return -1;

}

int config_get_path(char *path, unsigned int length, char *name)
{

    return snprintf(path, length, "%s/%s", root, name);

}

int config_get_subpath(char *path, unsigned int length, char *dir, char *name)
{

    return snprintf(path, length, "%s/%s/%s", root, dir, name);

}

int config_get_rundirshort(char *path, unsigned int length, char *id)
{

    return snprintf(path, length, "%s/%s/%c%c", root, CONFIG_LOGS, id[0], id[1]);

}

int config_get_rundirfull(char *path, unsigned int length, char *id)
{

    return snprintf(path, length, "%s/%s/%c%c/%s", root, CONFIG_LOGS, id[0], id[1], id);

}

int config_get_rundir(char *path, unsigned int length, char *id, int run)
{

    return snprintf(path, length, "%s/%s/%c%c/%s/%d", root, CONFIG_LOGS, id[0], id[1], id, run);

}

int config_get_runpath(char *path, unsigned int length, char *id, int run, char *name)
{

    return snprintf(path, length, "%s/%s/%c%c/%s/%d/%s", root, CONFIG_LOGS, id[0], id[1], id, run, name);

}

char *config_setup(void)
{

    char path[512];

    getcwd(path, 512);

    while (1)
    {    

        if (strlen(path) == 1)
            snprintf(root, BUFSIZ, "/%s", CONFIG_ROOT);
        else
            snprintf(root, BUFSIZ, "%s/%s", path, CONFIG_ROOT);

        if (access(root, F_OK) == 0)
            return root;

        if (strlen(path) == 1)
            break;

        dirname(path);

    }

    return 0;

}

