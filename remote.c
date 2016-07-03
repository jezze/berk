#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include "config.h"
#include "error.h"
#include "ini.h"
#include "remote.h"

static int getconfigpath(char *path, unsigned int length, char *filename)
{

    return snprintf(path, 1024, "%s/%s", BERK_REMOTES_BASE, filename) < 0;

}

static int loadcallback(void *user, const char *section, const char *name, const char *value)
{

    struct remote *remote = user;

    if (!strcmp(section, "remote") && !strcmp(name, "name"))
        remote->name = strdup(value);

    if (!strcmp(section, "remote") && !strcmp(name, "hostname"))
        remote->hostname = strdup(value);

    if (!strcmp(section, "remote") && !strcmp(name, "port"))
        remote->port = atoi(value);

    if (!strcmp(section, "remote") && !strcmp(name, "username"))
        remote->username = strdup(value);

    if (!strcmp(section, "remote") && !strcmp(name, "privatekey"))
        remote->privatekey = strdup(value);

    if (!strcmp(section, "remote") && !strcmp(name, "publickey"))
        remote->publickey = strdup(value);

    return 1;

}

int remote_load(char *filename, struct remote *remote)
{

    char path[1024];

    if (getconfigpath(path, 1024, filename))
        return -1;

    memset(remote, 0, sizeof (struct remote));

    return ini_parse(path, loadcallback, remote);

}

int remote_save(char *filename, struct remote *remote)
{

    FILE *file = fopen(filename, "w");

    if (file == NULL)
        return -1;

    ini_writesection(file, "remote");
    ini_writestring(file, "name", remote->name);
    ini_writestring(file, "hostname", remote->hostname);
    ini_writeint(file, "port", remote->port);
    ini_writestring(file, "username", remote->username);
    ini_writestring(file, "privatekey", remote->privatekey);
    ini_writestring(file, "publickey", remote->publickey);
    fclose(file);

    return 0;

}

void remote_copy(struct remote *remote, char *name)
{

    char path[1024];
    char *temp = remote->name;

    remote->name = name;

    getconfigpath(path, 1024, remote->name);

    if (remote_save(path, remote))
        error(ERROR_PANIC, "Could not save '%s'.", path);

    remote->name = temp;

    fprintf(stdout, "Remote '%s' (copied from '%s') created in '%s'\n", name, remote->name, path);

}

void remote_create(char *name, char *hostname, char *username)
{

    struct remote remote;
    char path[1024];
    char privatekey[512];
    char publickey[512];

    memset(&remote, 0, sizeof (struct remote));
    snprintf(privatekey, 512, "/home/%s/.ssh/%s", username, "id_rsa");
    snprintf(publickey, 512, "/home/%s/.ssh/%s", username, "id_rsa.pub");

    remote.name = name;
    remote.hostname = hostname;
    remote.port = 22;
    remote.username = username;
    remote.privatekey = privatekey;
    remote.publickey = publickey;

    getconfigpath(path, 1024, remote.name);

    if (remote_save(path, &remote))
        error(ERROR_PANIC, "Could not save '%s'.", path);

    fprintf(stdout, "Remote '%s' created in '%s'\n", remote.name, path);

}

void remote_list()
{

    DIR *dir;
    struct dirent *entry;

    dir = opendir(BERK_REMOTES_BASE);

    if (dir == NULL)
        error(ERROR_PANIC, "Could not open '%s'.", BERK_REMOTES_BASE);

    while ((entry = readdir(dir)) != NULL)
    {

        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        fprintf(stdout, "%s\n", entry->d_name);

    }

}

void remote_remove(struct remote *remote)
{

    char path[1024];

    getconfigpath(path, 1024, remote->name);

    if (unlink(path) < 0)
        error(ERROR_PANIC, "Could not remove '%s'.", path);

    fprintf(stdout, "Host '%s' removed from '%s'\n", remote->name, path);

}

void remote_show(struct remote *remote)
{

    fprintf(stdout, "name: %s\n", remote->name);
    fprintf(stdout, "hostname: %s\n", remote->hostname);
    fprintf(stdout, "port: %d\n", remote->port);
    fprintf(stdout, "privatekey: %s\n", remote->privatekey);
    fprintf(stdout, "publickey: %s\n", remote->publickey);

}

