#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <libssh2.h>
#include "config.h"
#include "util.h"
#include "ini.h"
#include "log.h"
#include "remote.h"

int remote_get_type(char *key)
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
        {"password", REMOTE_PASSWORD},
        {"privatekey", REMOTE_PRIVATEKEY},
        {"publickey", REMOTE_PUBLICKEY},
        {"tags", REMOTE_TAGS},
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

void *remote_get_value(struct remote *remote, int key)
{

    switch (key)
    {

    case REMOTE_NAME:
        return remote->name;

    case REMOTE_HOSTNAME:
        return remote->hostname;

    case REMOTE_PORT:
        return remote->port;

    case REMOTE_USERNAME:
        return remote->username;

    case REMOTE_PASSWORD:
        return remote->password;

    case REMOTE_PRIVATEKEY:
        return remote->privatekey;

    case REMOTE_PUBLICKEY:
        return remote->publickey;

    case REMOTE_TAGS:
        return remote->tags;

    }

    return NULL;

}

void *remote_set_value(struct remote *remote, int key, char *value)
{

    switch (key)
    {

    case REMOTE_NAME:
        return remote->name = strdup(value);

    case REMOTE_HOSTNAME:
        return remote->hostname = strdup(value);

    case REMOTE_PORT:
        return remote->port = strdup(value);

    case REMOTE_USERNAME:
        return remote->username = strdup(value);

    case REMOTE_PASSWORD:
        return remote->password = strdup(value);

    case REMOTE_PRIVATEKEY:
        return remote->privatekey = strdup(value);

    case REMOTE_PUBLICKEY:
        return remote->publickey = strdup(value);

    case REMOTE_TAGS:
        return remote->tags = strdup(value);

    }

    return NULL;

}

static int loadcallback(void *user, char *section, char *key, char *value)
{

    struct remote *remote = user;

    if (strcmp(section, "remote"))
        return 0;

    if (strlen(value))
        remote_set_value(remote, remote_get_type(key), value);

    return 0;

}

int remote_load(struct remote *remote)
{

    char path[BUFSIZ];

    config_get_subpath(path, BUFSIZ, CONFIG_REMOTES, remote->name);

    return ini_parse(path, loadcallback, remote);

}

int remote_save(struct remote *remote)
{

    FILE *file;
    char path[BUFSIZ];

    config_get_path(path, BUFSIZ, CONFIG_REMOTES);

    if (util_mkdir(path) < 0)
        return -1;

    config_get_subpath(path, BUFSIZ, CONFIG_REMOTES, remote->name);

    file = fopen(path, "w");

    if (file == NULL)
        return -1;

    ini_write_section(file, "remote");
    ini_write_string(file, "name", remote->name);
    ini_write_string(file, "hostname", remote->hostname);

    if (remote->port && strlen(remote->port))
        ini_write_string(file, "port", remote->port);

    if (remote->username && strlen(remote->username))
        ini_write_string(file, "username", remote->username);

    if (remote->password && strlen(remote->password))
        ini_write_string(file, "password", remote->password);

    if (remote->privatekey && strlen(remote->privatekey))
        ini_write_string(file, "privatekey", remote->privatekey);

    if (remote->publickey && strlen(remote->publickey))
        ini_write_string(file, "publickey", remote->publickey);

    if (remote->tags && strlen(remote->tags))
        ini_write_string(file, "tags", remote->tags);

    fclose(file);

    return 0;

}

int remote_erase(struct remote *remote)
{

    char path[BUFSIZ];

    config_get_subpath(path, BUFSIZ, CONFIG_REMOTES, remote->name);

    return util_unlink(path);

}

int remote_init_optional(struct remote *remote)
{

    char buffer[BUFSIZ];
    char keybuffer[BUFSIZ];
    struct passwd passwd, *current;

    if (!remote->port)
        remote_set_value(remote, REMOTE_PORT, "22");

    if (!remote->username)    
        remote_set_value(remote, REMOTE_USERNAME, getenv("USER"));

    if (getpwnam_r(remote->username, &passwd, buffer, BUFSIZ, &current))
        return 1;

    if (!remote->privatekey)
    {

        snprintf(keybuffer, BUFSIZ, "%s/.ssh/%s", passwd.pw_dir, "id_rsa");
        remote_set_value(remote, REMOTE_PRIVATEKEY, keybuffer);

    }

    if (!remote->publickey)
    {

        snprintf(keybuffer, BUFSIZ, "%s/.ssh/%s", passwd.pw_dir, "id_rsa.pub");
        remote_set_value(remote, REMOTE_PUBLICKEY, keybuffer);

    }

    return 0;

}

void remote_init(struct remote *remote, char *name)
{

    memset(remote, 0, sizeof (struct remote));
    remote_set_value(remote, REMOTE_NAME, name);

}

