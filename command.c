#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/stat.h>
#include "config.h"
#include "error.h"
#include "util.h"
#include "ini.h"
#include "remote.h"
#include "event.h"
#include "command.h"
#include "con.h"
#include "con_ssh.h"

void command_add(char *name, char *hostname, char *username)
{

    struct remote remote;
    char privatekey[512];
    char publickey[512];

    memset(&remote, 0, sizeof (struct remote));
    snprintf(privatekey, 512, "/home/%s/.ssh/%s", username, "id_rsa");
    snprintf(publickey, 512, "/home/%s/.ssh/%s", username, "id_rsa.pub");

    remote.name = name;
    remote.hostname = hostname;
    remote.port = "22";
    remote.username = username;
    remote.privatekey = privatekey;
    remote.publickey = publickey;

    if (remote_save(&remote))
        error(ERROR_PANIC, "Could not save '%s'.", remote.name);

    fprintf(stdout, "Remote '%s' added.\n", remote.name);

}

void command_config(struct remote *remote, char *key, char *value)
{

    if (!strcmp(key, "name"))
        remote->name = value;

    if (!strcmp(key, "hostname"))
        remote->hostname = value;

    if (!strcmp(key, "port"))
        remote->port = value;

    if (!strcmp(key, "username"))
        remote->username = value;

    if (!strcmp(key, "privatekey"))
        remote->privatekey = value;

    if (!strcmp(key, "publickey"))
        remote->publickey = value;

    if (!strcmp(key, "label"))
        remote->label = value;

    if (remote_save(remote))
        error(ERROR_PANIC, "Could not save '%s'.", remote->name);

}

int command_exec(struct remote *remote, unsigned int pid, char *command)
{

    int status;

    remote->pid = pid;

    remote_log_open(remote);

    if (event_start(remote))
        error(ERROR_PANIC, "Could not run event.");

    if (con_ssh_connect(remote) < 0)
        error(ERROR_PANIC, "Could not connect to remote '%s'.", remote->name);

    status = con_ssh_exec(remote, command);

    if (con_ssh_disconnect(remote) < 0)
        error(ERROR_PANIC, "Could not disconnect from remote '%s'.", remote->name);

    if (event_stop(remote, status))
        error(ERROR_PANIC, "Could not run event.");

    remote_log_close(remote);

    return status;

}

void command_init()
{

    FILE *file;
    char path[BUFSIZ];
    int fd;

    if (mkdir(CONFIG_ROOT, 0775) < 0)
        error(ERROR_PANIC, "Already initialized.");

    config_init();

    if (config_getpath(path, BUFSIZ, "config"))
        error(ERROR_PANIC, "Could not get path.");

    file = fopen(path, "w");

    if (file == NULL)
        error(ERROR_PANIC, "Could not create config file.");

    ini_writesection(file, "core");
    ini_writestring(file, "version", CONFIG_VERSION);
    fclose(file);

    if (config_getpath(path, BUFSIZ, CONFIG_REMOTES))
        error(ERROR_PANIC, "Could not get path.");

    if (mkdir(path, 0775) < 0)
        error(ERROR_PANIC, "Could not create directory '%s'.", CONFIG_REMOTES);

    if (config_getpath(path, BUFSIZ, CONFIG_LOGS))
        error(ERROR_PANIC, "Could not get path.");

    if (mkdir(path, 0775) < 0)
        error(ERROR_PANIC, "Could not create directory '%s'.", CONFIG_LOGS);

    if (config_getpath(path, BUFSIZ, CONFIG_HOOKS))
        error(ERROR_PANIC, "Could not get path.");

    if (mkdir(path, 0775) < 0)
        error(ERROR_PANIC, "Could not create directory '%s'.", CONFIG_HOOKS);

    if (config_getpath(path, BUFSIZ, CONFIG_HOOKS "/begin.sample"))
        error(ERROR_PANIC, "Could not get path.");

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0755);

    if (fd < 0)
        error(ERROR_PANIC, "Could not create config file.");

    file = fdopen(fd, "w");
    fprintf(file, "#!/bin/sh\n#\n# To enable this hook, rename this file to \"%s\".\n", "begin");
    fclose(file);
    close(fd);

    if (config_getpath(path, BUFSIZ, CONFIG_HOOKS "/end.sample"))
        error(ERROR_PANIC, "Could not get path.");

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0755);

    if (fd < 0)
        error(ERROR_PANIC, "Could not create config file.");

    file = fdopen(fd, "w");
    fprintf(file, "#!/bin/sh\n#\n# To enable this hook, rename this file to \"%s\".\n", "end");
    fclose(file);
    close(fd);

    if (config_getpath(path, BUFSIZ, CONFIG_HOOKS "/start.sample"))
        error(ERROR_PANIC, "Could not get path.");

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0755);

    if (fd < 0)
        error(ERROR_PANIC, "Could not create config file.");

    file = fdopen(fd, "w");
    fprintf(file, "#!/bin/sh\n#\n# To enable this hook, rename this file to \"%s\".\n", "start");
    fclose(file);
    close(fd);

    if (config_getpath(path, BUFSIZ, CONFIG_HOOKS "/stop.sample"))
        error(ERROR_PANIC, "Could not get path.");

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0755);

    if (fd < 0)
        error(ERROR_PANIC, "Could not create config file.");

    file = fdopen(fd, "w");
    fprintf(file, "#!/bin/sh\n#\n# To enable this hook, rename this file to \"%s\".\n", "stop");
    fclose(file);
    close(fd);
    fprintf(stdout, "Initialized %s in '%s'.\n", CONFIG_PROGNAME, CONFIG_ROOT);

}

void command_list(char *label)
{

    DIR *dir;
    char path[BUFSIZ];
    struct dirent *entry;

    if (config_getpath(path, BUFSIZ, CONFIG_REMOTES))
        error(ERROR_PANIC, "Could not get path.");

    dir = opendir(path);

    if (dir == NULL)
        error(ERROR_PANIC, "Could not open '%s'.", path);

    while ((entry = readdir(dir)) != NULL)
    {

        struct remote remote;
        unsigned int words;
        unsigned int i;

        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        if (remote_load(&remote, entry->d_name))
            continue;

        if (!label)
        {

            fprintf(stdout, "%s\n", remote.name);

            continue;

        }

        if (!remote.label)
            continue;

        words = util_split(remote.label);

        for (i = 0; (remote.label = util_nextword(remote.label, i, words)); i++)
        {

            if (!strcmp(remote.label, label))
            {

                fprintf(stdout, "%s\n", remote.name);

                break;

            }

        }

    }

}

void command_log(struct remote *remote, int pid)
{

    remote->pid = pid;

    remote_log_print(remote);

}

void command_remove(struct remote *remote)
{

    if (remote_erase(remote))
        error(ERROR_PANIC, "Could not remove '%s'.", remote->name);

    fprintf(stdout, "Remote '%s' removed.\n", remote->name);

}

void command_shell(struct remote *remote)
{

    struct termios old;
    struct termios new;

    if (con_ssh_connect(remote) < 0)
        error(ERROR_PANIC, "Could not connect to remote '%s'.", remote->name);

    tcgetattr(0, &old);
    cfmakeraw(&new);
    tcsetattr(0, TCSANOW, &new);
    con_ssh_shell(remote);
    tcsetattr(0, TCSANOW, &old);

    if (con_ssh_disconnect(remote) < 0)
        error(ERROR_PANIC, "Could not disconnect from remote '%s'.", remote->name);

}

void command_show(struct remote *remote, char *key)
{

    if (key)
    {

        if (!strcmp(key, "name"))
            fprintf(stdout, "%s\n", remote->name);

        if (!strcmp(key, "hostname"))
            fprintf(stdout, "%s\n", remote->hostname);

        if (!strcmp(key, "port"))
            fprintf(stdout, "%s\n", remote->port);

        if (!strcmp(key, "username"))
            fprintf(stdout, "%s\n", remote->username);

        if (!strcmp(key, "privatekey"))
            fprintf(stdout, "%s\n", remote->privatekey);

        if (!strcmp(key, "publickey"))
            fprintf(stdout, "%s\n", remote->publickey);

        if (!strcmp(key, "label"))
            fprintf(stdout, "%s\n", remote->label);

    }

    else
    {

        fprintf(stdout, "name=%s\n", remote->name);
        fprintf(stdout, "hostname=%s\n", remote->hostname);
        fprintf(stdout, "port=%s\n", remote->port ? remote->port : "");
        fprintf(stdout, "username=%s\n", remote->username ? remote->username : "");
        fprintf(stdout, "privatekey=%s\n", remote->privatekey ? remote->privatekey : "");
        fprintf(stdout, "publickey=%s\n", remote->publickey ? remote->publickey : "");
        fprintf(stdout, "label=%s\n", remote->label ? remote->label : "");

    }

}

