#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "config.h"
#include "error.h"
#include "util.h"
#include "remote.h"
#include "event.h"
#include "command.h"

struct command
{

    char *name;
    int (*parse)(int argc, char **argv);
    int argc;
    char *description;

};

static int errorremote(char *name)
{

    error(ERROR_NORMAL, "Could not find remote '%s'.", name);

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

    config_init();

    if (remote_add(name, hostname, username))
        error(ERROR_PANIC, "Could not add remote '%s'.", name);

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
            return errorremote(name);

        if (remote_config(&remote, key, value))
            error(ERROR_PANIC, "Could not run configure '%s'.", remote.name);

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

            if (remote_load(&remote, name))
                return errorremote(name);

            remote.pid = getpid();

            return command_exec(&remote, command);

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

    command_init();

    return EXIT_SUCCESS;

}

static int parselist(int argc, char **argv)
{

    char *label = (argc > 0) ? checkprint(argv[0]) : NULL;

    config_init();
    command_list(label);

    return EXIT_SUCCESS;

}

static int parselog(int argc, char **argv)
{

    char *name = checkprint(argv[0]);
    char *pid = checkdigit(argv[1]);
    struct remote remote;

    config_init();

    if (remote_load(&remote, name))
        return errorremote(name);

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
            return errorremote(name);

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
        return errorremote(name);

    command_shell(&remote);

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
            return errorremote(name);

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

