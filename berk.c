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

    error(ERROR_NORMAL, "Could not load remote '%s'.", name);

    return EXIT_FAILURE;

}

static int errorsave(char *name)
{

    error(ERROR_NORMAL, "Could not save remote '%s'.", name);

    return EXIT_FAILURE;

}

static int errorvalue(char *value)
{

    error(ERROR_NORMAL, "Could not parse value '%s'.", value);

    return EXIT_FAILURE;

}

static int checkargs(struct command *commands, int argc, char **argv)
{

    unsigned int i;

    if (!argc)
    {

        fprintf(stdout, "Usage: %s <command> [<args>]\n\n", CONFIG_PROGNAME);
        fprintf(stdout, "List of commands:\n");

        for (i = 0; commands[i].name; i++)
            fprintf(stdout, "    %s%s\n", commands[i].name, commands[i].description);

        return EXIT_SUCCESS;

    }

    for (i = 0; commands[i].name; i++)
    {

        if (strcmp(argv[0], commands[i].name))
            continue;

        if ((argc - 1) < commands[i].argc)
        {

            fprintf(stdout, "Usage: %s %s%s\n", CONFIG_PROGNAME, commands[i].name, commands[i].description);

            return EXIT_SUCCESS;

        }

        return commands[i].parse(argc - 1, argv + 1);

    }

    error(ERROR_NORMAL, "Invalid argument '%s'.", argv[0]);

    return EXIT_FAILURE;

}

static char *checkalpha(char *arg)
{

    util_trim(arg);

    if (util_checkalpha(arg))
        errorvalue(arg);

    return arg;

}

static char *checkdigit(char *arg)
{

    util_trim(arg);

    if (util_checkdigit(arg))
        errorvalue(arg);

    return arg;

}

static char *checkprint(char *arg)
{

    util_trim(arg);

    if (util_checkprint(arg))
        errorvalue(arg);

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
        error(ERROR_PANIC, "Could not init remote '%s'.", remote.name);

    if (remote_save(&remote))
        return errorsave(remote.name);

    fprintf(stdout, "Remote '%s' added.\n", name);

    return EXIT_SUCCESS;

}

static int parseconfig(int argc, char **argv)
{

    char *name = checklist(argv[0]);
    char *key = checkalpha(argv[1]);
    char *value = checkprint(argv[2]);
    struct remote remote;
    unsigned int names;
    unsigned int i;

    config_init();

    names = util_split(name);

    for (i = 0; (name = util_nextword(name, i, names)); i++)
    {

        if (util_checkprint(name))
            return errorvalue(name);

        if (remote_load(&remote, name))
            return errorload(name);

        if (remote_config(&remote, key, value))
            error(ERROR_PANIC, "Could not run configure '%s'.", remote.name);

        if (remote_save(&remote))
            return errorsave(remote.name);

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
    unsigned int total = 0;
    unsigned int complete = 0;
    unsigned int success = 0;
    unsigned int names;
    unsigned int i;
    int status;

    config_init();

    names = util_split(name);

    if (event_begin())
        error(ERROR_PANIC, "Could not run event.");

    for (i = 0; (name = util_nextword(name, i, names)); i++)
    {

        if (util_checkprint(name))
            return errorvalue(name);

        pid_t pid = fork();

        if (pid == 0)
        {

            struct remote remote;
            int rc;

            if (remote_load(&remote, name))
                return errorload(name);

            remote.pid = getpid();

            remote_log_open(&remote);

            if (event_start(&remote))
                error(ERROR_PANIC, "Could not run event.");

            if (con_ssh_connect(&remote) < 0)
                error(ERROR_PANIC, "Could not connect to remote '%s'.", remote.name);

            rc = con_ssh_exec(&remote, command);

            if (con_ssh_disconnect(&remote) < 0)
                error(ERROR_PANIC, "Could not disconnect from remote '%s'.", remote.name);

            if (event_stop(&remote, rc))
                error(ERROR_PANIC, "Could not run event.");

            remote_log_close(&remote);

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
        error(ERROR_PANIC, "Could not run event.");

    return EXIT_SUCCESS;

}

static int parseinit(int argc, char **argv)
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

    return EXIT_SUCCESS;

}

static int parselog(int argc, char **argv)
{

    char *name = checkprint(argv[0]);
    char *pid = checkdigit(argv[1]);
    struct remote remote;

    config_init();

    if (remote_load(&remote, name))
        return errorload(name);

    remote.pid = strtoul(pid, NULL, 10);

    if (!remote.pid)
        return errorvalue(pid);

    remote_log_print(&remote);

    return EXIT_SUCCESS;

}

static int parseremove(int argc, char **argv)
{

    char *name = checklist(argv[0]);
    struct remote remote;
    unsigned int names;
    unsigned int i;

    config_init();

    names = util_split(name);

    for (i = 0; (name = util_nextword(name, i, names)); i++)
    {

        if (util_checkprint(name))
            return errorvalue(name);

        if (remote_load(&remote, name))
            return errorload(name);

        if (remote_erase(&remote))
            error(ERROR_PANIC, "Could not remove '%s'.", remote.name);

        fprintf(stdout, "Remote '%s' removed.\n", remote.name);

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
        error(ERROR_PANIC, "Could not connect to remote '%s'.", remote.name);

    con_setraw();
    con_ssh_shell(&remote);
    con_unsetraw();

    if (con_ssh_disconnect(&remote) < 0)
        error(ERROR_PANIC, "Could not disconnect from remote '%s'.", remote.name);

    return EXIT_SUCCESS;

}

static int parseshow(int argc, char **argv)
{

    char *name = checklist(argv[0]);
    char *key = (argc > 1) ? checkalpha(argv[1]) : NULL;
    struct remote remote;
    unsigned int names;
    unsigned int i;

    config_init();

    names = util_split(name);

    for (i = 0; (name = util_nextword(name, i, names)); i++)
    {

        if (util_checkprint(name))
            return errorvalue(name);

        if (remote_load(&remote, name))
            return errorload(name);

        if (key)
        {

            char *value = remote_getvalue(&remote, key);

            if (!value)
                error(ERROR_PANIC, "Could not find key '%s'.", key);

            fprintf(stdout, "%s\n", value);

        }

        else
        {

            fprintf(stdout, "name=%s\n", remote.name);
            fprintf(stdout, "hostname=%s\n", remote.hostname);
            fprintf(stdout, "port=%s\n", remote.port ? remote.port : "");
            fprintf(stdout, "username=%s\n", remote.username ? remote.username : "");
            fprintf(stdout, "privatekey=%s\n", remote.privatekey ? remote.privatekey : "");
            fprintf(stdout, "publickey=%s\n", remote.publickey ? remote.publickey : "");
            fprintf(stdout, "label=%s\n", remote.label ? remote.label : "");

        }

    }

    return EXIT_SUCCESS;

}

static int parseversion(int argc, char **argv)
{

    fprintf(stdout, "%s: version %s\n", CONFIG_PROGNAME, CONFIG_VERSION);

    return EXIT_SUCCESS;

}

int main(int argc, char **argv)
{

    static struct command commands[] = {
        {"add", parseadd, 2, " <name> <hostname>"},
        {"config", parseconfig, 3, " <namelist> <key> <value>"},
        {"copy", parsecopy, 2, " <name>:<file> <name>:<file>"},
        {"exec", parseexec, 2, " <namelist> <command>"},
        {"init", parseinit, 0, ""},
        {"list", parselist, 0, " [<label>]"},
        {"log", parselog, 2, " <name> <pid>"},
        {"remove", parseremove, 1, " <namelist>"},
        {"send", parsesend, 2, " <namelist> <file>"},
        {"shell", parseshell, 1, " <name>"},
        {"show", parseshow, 1, " <namelist> [<key>]"},
        {"version", parseversion, 0, ""},
        {0}
    };

    return checkargs(commands, argc - 1, argv + 1);

}

