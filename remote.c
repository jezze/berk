#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include "config.h"
#include "ini.h"
#include "remote.h"

enum
{

    REMOTE_NAME,
    REMOTE_HOSTNAME,
    REMOTE_PORT,
    REMOTE_USERNAME,
    REMOTE_PRIVATEKEY,
    REMOTE_PUBLICKEY,
    REMOTE_LABEL

};

static int remote_gettype(char *key)
{

    static struct keynum
    {

        char *key;
        unsigned int value;

    } rkn[] = {
        {"name", REMOTE_NAME},
        {"hostname", REMOTE_HOSTNAME},
        {"port", REMOTE_PORT},
        {"username", REMOTE_USERNAME},
        {"privatekey", REMOTE_PRIVATEKEY},
        {"publickey", REMOTE_PUBLICKEY},
        {"label", REMOTE_LABEL},
        {0}
    };
    unsigned int i;

    for (i = 0; rkn[i].key; i++)
    {

        if (!strcmp(key, rkn[i].key))
            return rkn[i].value;

    }

    return -1;

}

void *remote_getvalue(struct remote *remote, char *key)
{

    switch (remote_gettype(key))
    {

    case REMOTE_NAME:
        return remote->name;

    case REMOTE_HOSTNAME:
        return remote->hostname;

    case REMOTE_PORT:
        return remote->port;

    case REMOTE_USERNAME:
        return remote->username;

    case REMOTE_PRIVATEKEY:
        return remote->privatekey;

    case REMOTE_LABEL:
        return remote->label;

    }

    return NULL;

}

static int loadcallback(void *user, char *section, char *key, char *value)
{

    struct remote *remote = user;

    if (strcmp(section, "remote"))
        return 0;

    switch (remote_gettype(key))
    {

    case REMOTE_NAME:
        remote->name = strdup(value);

        break;

    case REMOTE_HOSTNAME:
        remote->hostname = strdup(value);

        break;

    case REMOTE_PORT:
        remote->port = strdup(value);

        break;

    case REMOTE_USERNAME:
        remote->username = strdup(value);

        break;

    case REMOTE_PRIVATEKEY:
        remote->privatekey = strdup(value);

        break;

    case REMOTE_PUBLICKEY:
        remote->publickey = strdup(value);

        break;

    case REMOTE_LABEL:
        remote->label = strdup(value);

        break;

    }

    return 0;

}

int remote_load(struct remote *remote, char *name)
{

    char path[BUFSIZ];

    if (config_getremotepath(path, BUFSIZ, name))
        return -1;

    memset(remote, 0, sizeof (struct remote));

    return ini_parse(path, loadcallback, remote);

}

int remote_save(struct remote *remote)
{

    FILE *file;
    char path[BUFSIZ];

    if (config_getpath(path, BUFSIZ, CONFIG_REMOTES))
        return -1;

    if (access(path, F_OK) && mkdir(path, 0775) < 0)
        return -1;

    if (config_getremotepath(path, BUFSIZ, remote->name))
        return -1;

    file = fopen(path, "w");

    if (file == NULL)
        return -1;

    ini_writesection(file, "remote");
    ini_writestring(file, "name", remote->name);
    ini_writestring(file, "hostname", remote->hostname);

    if (remote->port)
        ini_writestring(file, "port", remote->port);

    if (remote->username)
        ini_writestring(file, "username", remote->username);

    if (remote->privatekey)
        ini_writestring(file, "privatekey", remote->privatekey);

    if (remote->publickey)
        ini_writestring(file, "publickey", remote->publickey);

    if (remote->label)
        ini_writestring(file, "label", remote->label);

    fclose(file);

    return 0;

}

int remote_erase(struct remote *remote)
{

    char path[BUFSIZ];

    if (config_getremotepath(path, BUFSIZ, remote->name))
        return -1;

    if (unlink(path) < 0)
        return -1;

    return 0;

}

int remote_init(struct remote *remote, char *name, char *hostname, char *username)
{

    struct passwd passwd, *current;
    char buffer[BUFSIZ];

    memset(remote, 0, sizeof (struct remote));
    loadcallback(remote, "remote", "name", name);
    loadcallback(remote, "remote", "hostname", hostname);
    loadcallback(remote, "remote", "port", "22");
    loadcallback(remote, "remote", "username", username);

    if (!getpwnam_r(username, &passwd, buffer, BUFSIZ, &current))
    {

        char privatekey[BUFSIZ];
        char publickey[BUFSIZ];

        snprintf(privatekey, BUFSIZ, "%s/.ssh/%s", passwd.pw_dir, "id_rsa");
        loadcallback(remote, "remote", "privatekey", privatekey);
        snprintf(publickey, BUFSIZ, "%s/.ssh/%s", passwd.pw_dir, "id_rsa.pub");
        loadcallback(remote, "remote", "publickey", publickey);

    }

    return 0;

}

int remote_config(struct remote *remote, char *key, char *value)
{

    switch (remote_gettype(key))
    {

    case REMOTE_NAME:
        remote->name = value;

        break;

    case REMOTE_HOSTNAME:
        remote->hostname = value;

        break;

    case REMOTE_PORT:
        remote->port = value;

        break;

    case REMOTE_USERNAME:
        remote->username = value;

        break;

    case REMOTE_PRIVATEKEY:
        remote->privatekey = value;

        break;

    case REMOTE_PUBLICKEY:
        remote->publickey = value;

        break;

    case REMOTE_LABEL:
        remote->label = value;

        break;

    }

    return 0;

}

int remote_loghead(int gid)
{

    char path[BUFSIZ];
    char buffer[BUFSIZ];
    unsigned int count;
    int fd;

    count = snprintf(buffer, BUFSIZ, "%d\n", gid);

    if (config_getpath(path, BUFSIZ, CONFIG_LOGS))
        return -1;

    if (access(path, F_OK) && mkdir(path, 0775) < 0)
        return -1;

    if (config_getpath(path, BUFSIZ, CONFIG_LOGS "/HEAD"))
        return -1;

    fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (fd < 0)
        return -1;

    write(fd, buffer, count);
    close(fd);

    return 0;

}

int remote_openlog(struct remote *remote)
{

    char path[BUFSIZ];

    if (config_getpath(path, BUFSIZ, CONFIG_LOGS))
        return -1;

    if (access(path, F_OK) && mkdir(path, 0775) < 0)
        return -1;

    if (config_getgroupbygid(path, BUFSIZ, remote->gid))
        return -1;

    if (access(path, F_OK) && mkdir(path, 0775) < 0)
        return -1;

    if (config_getprocessbypid(path, BUFSIZ, remote->gid, remote->pid))
        return -1;

    remote->logfd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    return remote->logfd;

}

void remote_closelog(struct remote *remote)
{

    close(remote->logfd);

}

int remote_log(struct remote *remote, char *buffer, unsigned int size)
{

    return write(remote->logfd, buffer, size);

}

