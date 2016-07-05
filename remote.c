#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include "config.h"
#include "error.h"
#include "ini.h"
#include "remote.h"

static int getremotepath(char *path, unsigned int length, char *filename)
{

    return snprintf(path, length, "%s/%s", CONFIG_REMOTES, filename) < 0;

}

static int getlogpath(char *path, unsigned int length, char *filename, unsigned int num)
{

    return snprintf(path, length, "%s/%s.%d", CONFIG_LOGS, filename, num) < 0;

}

static int loadcallback(void *user, const char *section, const char *name, const char *value)
{

    struct remote *remote = user;

    if (!strcmp(section, "remote") && !strcmp(name, "name"))
        remote->name = strdup(value);

    if (!strcmp(section, "remote") && !strcmp(name, "hostname"))
        remote->hostname = strdup(value);

    if (!strcmp(section, "remote") && !strcmp(name, "port"))
        remote->port = strdup(value);

    if (!strcmp(section, "remote") && !strcmp(name, "username"))
        remote->username = strdup(value);

    if (!strcmp(section, "remote") && !strcmp(name, "privatekey"))
        remote->privatekey = strdup(value);

    if (!strcmp(section, "remote") && !strcmp(name, "publickey"))
        remote->publickey = strdup(value);

    return 0;

}

int remote_load(struct remote *remote, char *name)
{

    char path[BUFSIZ];

    if (getremotepath(path, BUFSIZ, name))
        return -1;

    memset(remote, 0, sizeof (struct remote));

    return ini_parse(path, loadcallback, remote);

}

int remote_save(struct remote *remote)
{

    FILE *file;
    char path[BUFSIZ];

    if (getremotepath(path, BUFSIZ, remote->name))
        return -1;

    file = fopen(path, "w");

    if (file == NULL)
        return -1;

    ini_writesection(file, "remote");
    ini_writestring(file, "name", remote->name);
    ini_writestring(file, "hostname", remote->hostname);
    ini_writestring(file, "port", remote->port);
    ini_writestring(file, "username", remote->username);
    ini_writestring(file, "privatekey", remote->privatekey);
    ini_writestring(file, "publickey", remote->publickey);
    fclose(file);

    return 0;

}

int remote_erase(struct remote *remote)
{

    char path[BUFSIZ];

    if (getremotepath(path, BUFSIZ, remote->name))
        return -1;

    if (unlink(path) < 0)
        return -1;

    return 0;

}

int remote_log_open(struct remote *remote)
{

    char path[BUFSIZ];

    if (getlogpath(path, BUFSIZ, remote->name, remote->pid))
        return -1;

    remote->logfd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    return remote->logfd;

}

void remote_log_close(struct remote *remote)
{

    close(remote->logfd);

}

int remote_log(struct remote *remote, char *buffer, unsigned int size)
{

    return write(remote->logfd, buffer, size);

}

int remote_log_print(struct remote *remote)
{

    char path[BUFSIZ];
    char buffer[BUFSIZ];
    unsigned int count;

    if (getlogpath(path, BUFSIZ, remote->name, remote->pid))
        return -1;

    remote->logfd = open(path, O_RDONLY, 0644);

    if (remote->logfd < 0)
        return -1;

    while ((count = read(remote->logfd, buffer, BUFSIZ)))
        write(1, buffer, count);

    close(remote->logfd);

    return 0;

}

