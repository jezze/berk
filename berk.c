#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "config.h"
#include "error.h"
#include "util.h"
#include "ini.h"
#include "remote.h"
#include "event.h"
#include "con.h"
#include "con_ssh.h"

struct command
{

    char *name;
    int (*parse)(int argc, char **argv);
    int argc;
    char *description;

};

static int errorload(char *name)
{

    return error(ERROR_NORMAL, "Could not load remote '%s'.", name);

}

static int errorsave(char *name)
{

    return error(ERROR_NORMAL, "Could not save remote '%s'.", name);

}

static int errorparse(char *value)
{

    return error(ERROR_NORMAL, "Could not parse value '%s'.", value);

}

static int checkargs(struct command *commands, int argc, char **argv)
{

    unsigned int i;

    if (!argc)
    {

        printf("Usage: %s <command> [<args>]\n\n", CONFIG_PROGNAME);
        printf("List of commands:\n");

        for (i = 0; commands[i].name; i++)
            printf("    %s%s\n", commands[i].name, commands[i].description);

        return EXIT_SUCCESS;

    }

    for (i = 0; commands[i].name; i++)
    {

        if (strcmp(argv[0], commands[i].name))
            continue;

        if ((argc - 1) < commands[i].argc)
        {

            printf("Usage: %s %s%s\n", CONFIG_PROGNAME, commands[i].name, commands[i].description);

            return EXIT_SUCCESS;

        }

        return commands[i].parse(argc - 1, argv + 1);

    }

    return error(ERROR_NORMAL, "Invalid argument '%s'.", argv[0]);

}

static char *checkalpha(char *arg)
{

    util_trim(arg);

    if (util_checkalpha(arg))
        errorparse(arg);

    return arg;

}

static char *checkdigit(char *arg)
{

    util_trim(arg);

    if (util_checkdigit(arg))
        errorparse(arg);

    return arg;

}

static char *checkprint(char *arg)
{

    util_trim(arg);

    if (util_checkprint(arg))
        errorparse(arg);

    return arg;

}

static char *checklist(char *arg)
{

    util_trim(arg);
    util_strip(arg);

    return arg;

}

static int parseadd(int argc, char **argv)
{

    char *name = checkprint(argv[0]);
    char *hostname = checkprint(argv[1]);
    char *username = getenv("USER");
    struct remote remote;

    config_init();

    if (remote_init(&remote, name, hostname, username))
        return error(ERROR_NORMAL, "Could not init remote '%s'.", name);

    if (remote_save(&remote))
        return errorsave(name);

    printf("Remote '%s' added.\n", name);

    return EXIT_SUCCESS;

}

static int parseconfig(int argc, char **argv)
{

    char *name = checklist(argv[0]);
    char *key = (argc > 1) ? checkalpha(argv[1]) : NULL;
    char *value = (argc > 2) ? checkprint(argv[2]) : NULL;
    unsigned int names = util_split(name);
    struct remote remote;
    unsigned int i;

    config_init();

    for (i = 0; (name = util_nextword(name, i, names)); i++)
    {

        if (util_checkprint(name))
            return errorparse(name);

        if (remote_load(&remote, name))
            return errorload(name);

        if (value)
        {

            if (remote_config(&remote, key, value))
                return error(ERROR_NORMAL, "Could not run configure remote '%s'.", remote.name);

            if (remote_save(&remote))
                return errorsave(name);

        }

        else
        {

            if (key)
            {

                char *value = remote_getvalue(&remote, key);

                if (!value)
                    return error(ERROR_NORMAL, "Could not find key '%s'.", key);

                printf("%s\n", value);

            }

            else
            {

                printf("name=%s\n", remote.name);
                printf("hostname=%s\n", remote.hostname);
                printf("port=%s\n", remote.port ? remote.port : "");
                printf("username=%s\n", remote.username ? remote.username : "");
                printf("privatekey=%s\n", remote.privatekey ? remote.privatekey : "");
                printf("publickey=%s\n", remote.publickey ? remote.publickey : "");
                printf("label=%s\n", remote.label ? remote.label : "");

            }

        }

    }

    return EXIT_SUCCESS;

}

static int parsecopy(int argc, char **argv)
{

    config_init();
    error(ERROR_NORMAL, "Copy not implemented.");

    return EXIT_SUCCESS;

}

static int parseexec(int argc, char **argv)
{

    char *name = checklist(argv[0]);
    char *command = checkprint(argv[1]);
    unsigned int names = util_split(name);
    unsigned int total = 0;
    unsigned int complete = 0;
    unsigned int success = 0;
    unsigned int i;
    int status;

    config_init();

    if (event_begin())
        return error(ERROR_NORMAL, "Could not run event.");

    for (i = 0; (name = util_nextword(name, i, names)); i++)
    {

        if (util_checkprint(name))
            return errorparse(name);

        pid_t pid = fork();

        if (pid == 0)
        {

            struct remote remote;
            int rc;

            if (remote_load(&remote, name))
                return errorload(name);

            remote.pid = getpid();

            remote_openlog(&remote);

            if (event_start(&remote))
                return error(ERROR_NORMAL, "Could not run event.");

            if (con_ssh_connect(&remote) < 0)
                return error(ERROR_NORMAL, "Could not connect to remote '%s'.", remote.name);

            rc = con_ssh_exec(&remote, command);

            if (con_ssh_disconnect(&remote) < 0)
                return error(ERROR_NORMAL, "Could not disconnect from remote '%s'.", remote.name);

            if (event_stop(&remote, rc))
                return error(ERROR_NORMAL, "Could not run event.");

            remote_closelog(&remote);

            return rc;

        }

    }

    while (wait(&status) > 0)
    {

        total++;

        if (WIFEXITED(status))
        {

            complete++;

            if (WEXITSTATUS(status) == 0)
                success++;

        }

    }

    if (event_end(total, complete, success))
        return error(ERROR_NORMAL, "Could not run event.");

    return EXIT_SUCCESS;

}

static int parseinit(int argc, char **argv)
{

    FILE *file;
    char path[BUFSIZ];
    char *hooks[] = {"begin", "end", "start", "stop", 0};
    unsigned int i;
    int fd;

    if (mkdir(CONFIG_ROOT, 0775) < 0)
        return error(ERROR_NORMAL, "Already initialized.");

    config_init();

    if (config_getpath(path, BUFSIZ, "config"))
        return error(ERROR_NORMAL, "Could not get path.");

    file = fopen(path, "w");

    if (file == NULL)
        return error(ERROR_NORMAL, "Could not create config file.");

    ini_writesection(file, "core");
    ini_writestring(file, "version", CONFIG_VERSION);
    fclose(file);

    if (config_getpath(path, BUFSIZ, CONFIG_HOOKS))
        return error(ERROR_NORMAL, "Could not get path.");

    if (mkdir(path, 0775) < 0)
        return error(ERROR_NORMAL, "Could not create directory '%s'.", CONFIG_HOOKS);

    for (i = 0; hooks[i]; i++)
    {

        char buffer[BUFSIZ];
        unsigned int count;

        snprintf(buffer, BUFSIZ, "%s/%s.sample", CONFIG_HOOKS, hooks[i]);

        if (config_getpath(path, BUFSIZ, buffer))
            return error(ERROR_NORMAL, "Could not get path.");

        fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0755);

        if (fd < 0)
            return error(ERROR_NORMAL, "Could not create hook file '%s'.", hooks[i]);

        count = snprintf(buffer, BUFSIZ, "#!/bin/sh\n#\n# To enable this hook, rename this file to \"%s\".\n", hooks[i]);

        write(fd, buffer, count);
        close(fd);

    }

    printf("Initialized %s in '%s'.\n", CONFIG_PROGNAME, CONFIG_ROOT);

    return EXIT_SUCCESS;

}

static int parselist(int argc, char **argv)
{

    char *label = (argc > 0) ? checkprint(argv[0]) : NULL;
    DIR *dir;
    struct dirent *entry;
    char path[BUFSIZ];

    config_init();

    if (config_getpath(path, BUFSIZ, CONFIG_REMOTES))
        return error(ERROR_NORMAL, "Could not get path.");

    dir = opendir(path);

    if (dir == NULL)
        return error(ERROR_NORMAL, "Could not open '%s'.", path);

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

            printf("%s\n", remote.name);

            continue;

        }

        if (!remote.label)
            continue;

        words = util_split(remote.label);

        for (i = 0; (remote.label = util_nextword(remote.label, i, words)); i++)
        {

            if (!strcmp(remote.label, label))
            {

                printf("%s\n", remote.name);

                break;

            }

        }

    }

    return EXIT_SUCCESS;

}

static int parselog(int argc, char **argv)
{

    char *pid = checkdigit(argv[0]);
    char path[BUFSIZ];
    char buffer[BUFSIZ];
    unsigned int count;
    int fd;

    config_init();

    if (config_getlogpathbyname(path, BUFSIZ, pid))
        return error(ERROR_NORMAL, "Could not get path.");

    fd = open(path, O_RDONLY, 0644);

    if (fd < 0)
        return error(ERROR_NORMAL, "Could not open '%s'.", path);

    while ((count = read(fd, buffer, BUFSIZ)))
        write(STDOUT_FILENO, buffer, count);

    close(fd);

    return EXIT_SUCCESS;

}

static int parseremove(int argc, char **argv)
{

    char *name = checklist(argv[0]);
    unsigned int names = util_split(name);
    struct remote remote;
    unsigned int i;

    config_init();

    for (i = 0; (name = util_nextword(name, i, names)); i++)
    {

        if (util_checkprint(name))
            return errorparse(name);

        if (remote_load(&remote, name))
            return errorload(name);

        if (remote_erase(&remote))
            return error(ERROR_NORMAL, "Could not remove remote '%s'.", remote.name);

        printf("Remote '%s' removed.\n", remote.name);

    }

    return EXIT_SUCCESS;

}

static int parsesend(int argc, char **argv)
{

    config_init();
    error(ERROR_NORMAL, "Send not implemented.");

    return EXIT_SUCCESS;

}

static int parseshell(int argc, char **argv)
{

    char *name = checkprint(argv[0]);
    struct remote remote;

    config_init();

    if (remote_load(&remote, name))
        return errorload(name);

    if (con_ssh_connect(&remote) < 0)
        return error(ERROR_NORMAL, "Could not connect to remote '%s'.", remote.name);

    con_setraw();
    con_ssh_shell(&remote);
    con_unsetraw();

    if (con_ssh_disconnect(&remote) < 0)
        return error(ERROR_NORMAL, "Could not disconnect from remote '%s'.", remote.name);

    return EXIT_SUCCESS;

}

static int parseversion(int argc, char **argv)
{

    printf("%s version %s\n", CONFIG_PROGNAME, CONFIG_VERSION);

    return EXIT_SUCCESS;

}

int main(int argc, char **argv)
{

    static struct command commands[] = {
        {"add", parseadd, 2, " <name> <hostname>"},
        {"config", parseconfig, 1, " <namelist> [<key>] [<value>]"},
        {"copy", parsecopy, 2, " <name:file> <name:file>"},
        {"exec", parseexec, 2, " <namelist> <command>"},
        {"init", parseinit, 0, ""},
        {"list", parselist, 0, " [<label>]"},
        {"log", parselog, 1, " <pid>"},
        {"remove", parseremove, 1, " <namelist>"},
        {"send", parsesend, 2, " <namelist> <file>"},
        {"shell", parseshell, 1, " <name>"},
        {"version", parseversion, 0, ""},
        {0}
    };

    return checkargs(commands, argc - 1, argv + 1);

}

