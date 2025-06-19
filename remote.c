#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <libssh2.h>
#include "config.h"
#include "util.h"
#include "ini.h"
#include "log.h"
#include "run.h"
#include "remote.h"
#include "ssh.h"

void *remote_get_value(struct remote *remote, unsigned int hash)
{

    switch (hash)
    {

    case REMOTE_NAME:
        return remote->name;

    case REMOTE_TYPE:
        return remote->type;

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

unsigned int remote_set_value(struct remote *remote, unsigned int hash, char *value)
{

    char *v = (value != NULL) ? strdup(value) : NULL;

    switch (hash)
    {

    case REMOTE_NAME:
        remote->name = v;

        break;

    case REMOTE_TYPE:
        remote->type = v;

        break;

    case REMOTE_HOSTNAME:
        remote->hostname = v;

        break;

    case REMOTE_PORT:
        remote->port = v;

        break;

    case REMOTE_USERNAME:
        remote->username = v;

        break;

    case REMOTE_PASSWORD:
        remote->password = v;

        break;

    case REMOTE_PRIVATEKEY:
        remote->privatekey = v;

        break;

    case REMOTE_PUBLICKEY:
        remote->publickey = v;

        break;

    case REMOTE_TAGS:
        remote->tags = v;

        break;

    default:
        hash = 0;

        break;

    }

    return hash;

}

static int loadcallback(void *user, char *section, char *key, char *value)
{

    struct remote *remote = user;
    unsigned int hash = util_hash(key);

    if (strcmp(section, "remote"))
        return 0;

    if (strlen(value))
        remote_set_value(remote, hash, value);

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

    char path[BUFSIZ];
    int fd;

    config_get_path(path, BUFSIZ, CONFIG_REMOTES);

    if (util_mkdir(path) < 0)
        return -1;

    config_get_subpath(path, BUFSIZ, CONFIG_REMOTES, remote->name);

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd < 0)
        return -1;

    ini_write_section(fd, "remote");

    if (remote->name && strlen(remote->name))
        ini_write_string(fd, "name", remote->name);

    if (remote->type && strlen(remote->type))
        ini_write_string(fd, "type", remote->type);

    if (remote->hostname && strlen(remote->hostname))
        ini_write_string(fd, "hostname", remote->hostname);

    if (remote->port && strlen(remote->port))
        ini_write_string(fd, "port", remote->port);

    if (remote->username && strlen(remote->username))
        ini_write_string(fd, "username", remote->username);

    if (remote->password && strlen(remote->password))
        ini_write_string(fd, "password", remote->password);

    if (remote->privatekey && strlen(remote->privatekey))
        ini_write_string(fd, "privatekey", remote->privatekey);

    if (remote->publickey && strlen(remote->publickey))
        ini_write_string(fd, "publickey", remote->publickey);

    if (remote->tags && strlen(remote->tags))
        ini_write_string(fd, "tags", remote->tags);

    close(fd);

    return 0;

}

int remote_erase(struct remote *remote)
{

    char path[BUFSIZ];

    config_get_subpath(path, BUFSIZ, CONFIG_REMOTES, remote->name);

    return util_unlink(path);

}

int remote_prepare(struct remote *remote)
{

    if (!strcmp(remote->type, "local"))
    {


    }

    else
    {

        char buffer[BUFSIZ];
        char keybuffer[BUFSIZ];
        struct passwd passwd, *current;

        if (!remote->hostname)    
            remote_set_value(remote, REMOTE_HOSTNAME, "localhost");

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

    }

    return 0;

}

int remote_connect(struct remote *remote)
{

    if (!strcmp(remote->type, "local"))
        return 0;
    else
        return ssh_connect(remote);

}

int remote_disconnect(struct remote *remote)
{

    if (!strcmp(remote->type, "local"))
        return 0;
    else
        return ssh_disconnect(remote);

}

int remote_exec(struct remote *remote, struct run *run, char *command)
{

    if (!strcmp(remote->type, "local"))
    {

        int oldstderrfd = dup(STDERR_FILENO);
        int oldstdoutfd = dup(STDOUT_FILENO);
        int rc;

        dup2(run->stderrfd, STDERR_FILENO);
        dup2(run->stdoutfd, STDOUT_FILENO);

        rc = system(command);

        dup2(oldstderrfd, STDERR_FILENO);
        dup2(oldstdoutfd, STDOUT_FILENO);

        return rc;

    }

    else
    {

        return ssh_exec(remote, run, command);

    }

}

int remote_send(struct remote *remote, char *localpath, char *remotepath)
{

    if (!strcmp(remote->type, "local"))
    {

        struct stat fileinfo;
        char buffer[BUFSIZ];
        size_t total;
        int fdlocal;
        int fdremote;

        if (stat(localpath, &fileinfo))
            return -1;

        fdlocal = open(localpath, O_RDONLY, 0644);

        if (fdlocal < 0)
            return -1;

        fdremote = open(remotepath, O_WRONLY | O_CREAT, fileinfo.st_mode | 0777);

        if (fdremote < 0)
            return -1;

        while ((total = read(fdlocal, buffer, BUFSIZ)))
        {

            char *current = buffer;

            if (total < 0)
                break;

            do
            {

                int count = write(fdremote, current, total);

                if (count < 0)
                    break;

                current += count;
                total -= count;

            } while (total);
     
        };

        close(fdlocal);
        close(fdremote);

        return 0;

    }

    else
    {

        return ssh_send(remote, localpath, remotepath);

    }

}

void remote_init(struct remote *remote, char *name)
{

    memset(remote, 0, sizeof (struct remote));
    remote_set_value(remote, REMOTE_NAME, name);

}

