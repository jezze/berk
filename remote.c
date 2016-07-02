#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include "berk.h"
#include "ini.h"
#include "init.h"
#include "remote.h"

void remote_add(char *remote_name, char *remote_hostname, char *remote_username)
{

    static const char *fmt =
        "[remote]\n"
        "        name = %s\n"
        "        hostname = %s\n"
        "        port = 22\n"
        "        username = %s\n"
        "        publickey = /home/%s/.ssh/id_rsa.pub\n"
        "        privatekey = /home/%s/.ssh/id_rsa\n"
        "        label = all\n";
    char path[1024];
    FILE *config;

    init_assert();

    if (snprintf(path, 1024, "%s/%s", BERK_REMOTES_BASE, remote_name) < 0)
        berk_panic("Could not copy string.");

    config = fopen(path, "w");

    if (config == NULL)
        berk_panic("Could not create remote config file.");

    if (remote_username == NULL)
        remote_username = "berk";

    if (fprintf(config, fmt, remote_name, remote_hostname, remote_username, remote_username, remote_username) < 0)
        berk_panic("Could not write to remote config file.");

    fclose(config);
    fprintf(stdout, "Host '%s' created in '%s'\n", remote_name, path);

}

void remote_copy(char *old_remote_name, char *new_remote_name)
{

}

void remote_disable(char *remote_name)
{

    struct remote remote;

    init_assert();

    if (remote_load(remote_name, &remote) < 0)
        berk_panic("Could not load remote.");

}

void remote_enable(char *remote_name)
{

    struct remote remote;

    init_assert();

    if (remote_load(remote_name, &remote) < 0)
        berk_panic("Could not load remote.");

}

void remote_list()
{

    DIR *dir;
    struct dirent *entry;

    init_assert();

    dir = opendir(BERK_REMOTES_BASE);

    if (dir == NULL)
        berk_panic("Could not open dir.");

    while ((entry = readdir(dir)) != NULL)
    {

        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        fprintf(stdout, "%s\n", entry->d_name);

    }

}

int remote_load_callback(void *user, const char *section, const char *name, const char *value)
{

    struct remote *remote = user;

    if (!strcmp(section, "remote") && !strcmp(name, "name"))
        strncpy(remote->name, value, 32);

    if (!strcmp(section, "remote") && !strcmp(name, "hostname"))
        strncpy(remote->hostname, value, 64);

    if (!strcmp(section, "remote") && !strcmp(name, "port"))
        remote->port = atoi(value);

    if (!strcmp(section, "remote") && !strcmp(name, "username"))
        strncpy(remote->username, value, 64);

    if (!strcmp(section, "remote") && !strcmp(name, "publickey"))
        strncpy(remote->publickey, value, 64);

    if (!strcmp(section, "remote") && !strcmp(name, "privatekey"))
        strncpy(remote->privatekey, value, 64);

    return 1;

}

int remote_load(char *remote_name, struct remote *remote)
{

    char path[1024];

    if (snprintf(path, 1024, "%s/%s", BERK_REMOTES_BASE, remote_name) < 0)
        return -1;

    memset(remote, 0, sizeof (struct remote));

    return ini_parse(path, remote_load_callback, remote);

}

void remote_remove(char *remote_name)
{

    char path[1024];

    init_assert();

    if (snprintf(path, 1024, "%s/%s", BERK_REMOTES_BASE, remote_name) < 0)
        berk_panic("Could not copy string.");

    if (unlink(path) < 0)
        berk_panic("Could not remove file.");

    fprintf(stdout, "Host '%s' removed from '%s'\n", remote_name, path);

}

static int remote_parse_add(int argc, char **argv)
{

    if (argc < 2)
        return berk_error_missing();

    remote_add(argv[0], argv[1], getenv("USER"));

    return EXIT_SUCCESS;

}

static int remote_parse_copy(int argc, char **argv)
{

    if (argc < 2)
        return berk_error_missing();

    remote_copy(argv[0], argv[1]);

    return EXIT_SUCCESS;

}

static int remote_parse_disable(int argc, char **argv)
{

    if (argc < 1)
        return berk_error_missing();

    remote_disable(argv[0]);

    return EXIT_SUCCESS;

}

static int remote_parse_enable(int argc, char **argv)
{

    if (argc < 1)
        return berk_error_missing();

    remote_enable(argv[0]);

    return EXIT_SUCCESS;

}

static int remote_parse_list(int argc, char **argv)
{

    if (argc)
        return berk_error_extra();

    remote_list();

    return EXIT_SUCCESS;

}

static int remote_parse_remove(int argc, char **argv)
{

    if (argc < 1)
        return berk_error_missing();

    remote_remove(argv[0]);

    return EXIT_SUCCESS;

}

int remote_parse(int argc, char **argv)
{

    static struct berk_command cmds[] = {
        {"add", remote_parse_add, "<remote-name> <remote-hostname>"},
        {"copy", remote_parse_copy, "<old-remote-name> <new-remote-name>"},
        {"disable", remote_parse_disable, "<remote-name>"},
        {"enable", remote_parse_enable, "<remote-name>"},
        {"list", remote_parse_list, 0},
        {"remove", remote_parse_remove, "<remote-name>"},
        {0}
    };
    unsigned int i;

    if (argc < 1)
        return berk_error_command("remote", cmds);

    for (i = 0; cmds[i].name; i++)
    {

        if (strcmp(argv[0], cmds[i].name))
            continue;

        return cmds[i].parse(argc - 1, argv + 1);

    }

    return berk_error_invalid(argv[0]);

}

