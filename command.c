#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <termios.h>
#include <sys/stat.h>
#include "config.h"
#include "error.h"
#include "ini.h"
#include "remote.h"
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
    remote.port = 22;
    remote.username = username;
    remote.privatekey = privatekey;
    remote.publickey = publickey;

    if (remote_save(&remote))
        error(ERROR_PANIC, "Could not save '%s'.", remote.name);

    fprintf(stdout, "Remote '%s' added.\n", remote.name);

}

void command_clone(struct remote *remote, char *name)
{

    char *temp = remote->name;

    remote->name = name;

    if (remote_save(remote))
        error(ERROR_PANIC, "Could not save '%s'.", remote->name);

    remote->name = temp;

    fprintf(stdout, "Remote '%s' (clone of '%s') added.\n", name, remote->name);

}

void command_config(struct remote *remote, char *key, char *value)
{

    if (!strcmp(key, "name"))
        remote->name = value;

    if (!strcmp(key, "hostname"))
        remote->hostname = value;

    if (!strcmp(key, "port"))
    {

        remote->port = strtoul(value, NULL, 10);

        if (!remote->port)
            error(ERROR_PANIC, "Invalid port '%d'.", remote->port);

    }

    if (!strcmp(key, "username"))
        remote->username = value;

    if (!strcmp(key, "privatekey"))
        remote->privatekey = value;

    if (!strcmp(key, "publickey"))
        remote->publickey = value;

    if (remote_save(remote))
        error(ERROR_PANIC, "Could not save '%s'.", remote->name);

}

int command_exec(struct remote *remote, unsigned int pid, char *command)
{

    int status;

    remote->pid = pid;

    remote_log_open(remote);
    fprintf(stdout, "event=start name=%s pid=%d\n", remote->name, remote->pid);

    if (con_ssh_connect(remote) < 0)
        error(ERROR_PANIC, "Could not connect to remote '%s'.", remote->name);

    status = con_ssh_exec(remote, command);

    if (con_ssh_disconnect(remote) < 0)
        error(ERROR_PANIC, "Could not disconnect from remote '%s'.", remote->name);

    fprintf(stdout, "event=stop name=%s pid=%d status=%d\n", remote->name, remote->pid, status);
    remote_log_close(remote);

    return status;

}

void command_init()
{

    FILE *file;
    char path[BUFSIZ];

    if (mkdir(CONFIG_ROOT, 0775) < 0)
        error(ERROR_PANIC, "Already initialized.");

    if (snprintf(path, BUFSIZ, "%s", CONFIG_MAIN) < 0)
        error(ERROR_PANIC, "Could not copy string.");

    file = fopen(path, "w");

    if (file == NULL)
        error(ERROR_PANIC, "Could not create config file.");

    ini_writesection(file, "core");
    ini_writestring(file, "version", CONFIG_VERSION);
    fclose(file);

    if (snprintf(path, BUFSIZ, "%s", CONFIG_REMOTES) < 0)
        error(ERROR_PANIC, "Could not copy string.");

    if (mkdir(path, 0775) < 0)
        error(ERROR_PANIC, "Could not create directory.");

    if (snprintf(path, BUFSIZ, "%s", CONFIG_LOGS) < 0)
        error(ERROR_PANIC, "Could not copy string.");

    if (mkdir(path, 0775) < 0)
        error(ERROR_PANIC, "Could not create directory.");

    fprintf(stdout, "Initialized %s in '%s'.\n", CONFIG_PROGNAME, CONFIG_ROOT);

}

void command_list()
{

    DIR *dir;
    struct dirent *entry;

    dir = opendir(CONFIG_REMOTES);

    if (dir == NULL)
        error(ERROR_PANIC, "Could not open '%s'.", CONFIG_REMOTES);

    while ((entry = readdir(dir)) != NULL)
    {

        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        fprintf(stdout, "%s\n", entry->d_name);

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

void command_show(struct remote *remote)
{

    fprintf(stdout, "name=%s\n", remote->name);
    fprintf(stdout, "hostname=%s\n", remote->hostname);
    fprintf(stdout, "port=%d\n", remote->port);
    fprintf(stdout, "username=%s\n", remote->username);
    fprintf(stdout, "privatekey=%s\n", remote->privatekey);
    fprintf(stdout, "publickey=%s\n", remote->publickey);

}

