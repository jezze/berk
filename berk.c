#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "config.h"
#include "error.h"
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

static void checkinit()
{

    if (access(CONFIG_MAIN, F_OK) < 0)
        error(ERROR_PANIC, "Could not find '%s' directory.", CONFIG_ROOT);

}

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

static int parseadd(int argc, char **argv)
{

    checkinit();
    command_add(argv[0], argv[1], getenv("USER"));

    return EXIT_SUCCESS;

}

static int parseconfig(int argc, char **argv)
{

    struct remote remote;

    checkinit();

    if (remote_load(&remote, argv[0]))
        return errorremote(argv[0]);

    command_config(&remote, argv[1], argv[2]);

    return EXIT_SUCCESS;

}

static int parseclone(int argc, char **argv)
{

    struct remote remote;

    checkinit();

    if (remote_load(&remote, argv[0]))
        return errorremote(argv[0]);

    command_clone(&remote, argv[1]);

    return EXIT_SUCCESS;

}

static int parseexec(int argc, char **argv)
{

    unsigned int complete = 0;
    unsigned int success = 0;
    unsigned int total;
    unsigned int i;
    int status;

    checkinit();

    total = strtoul(argv[1], NULL, 10);

    if (!total)
        return errorvalue(argv[1]);

    event_begin(total);

    for (i = 0; i < total; i++)
    {

        pid_t pid = fork();

        if (pid == 0)
        {

            struct remote remote;

            if (remote_load(&remote, argv[0]))
                return errorremote(argv[0]);

            return command_exec(&remote, getpid(), argv[2]);

        }

    }

    while (wait(&status) > 0)
    {

        if (WIFEXITED(status))
        {

            complete++;

            if (WEXITSTATUS(status) == 0)
                success++;

        }

    }

    event_end(total, complete, success);

    return EXIT_SUCCESS;

}

static int parseinit(int argc, char **argv)
{

    command_init();

    return EXIT_SUCCESS;

}

static int parselist(int argc, char **argv)
{

    checkinit();
    command_list();

    return EXIT_SUCCESS;

}

static int parselog(int argc, char **argv)
{

    struct remote remote;
    unsigned int pid;

    checkinit();

    if (remote_load(&remote, argv[0]))
        return errorremote(argv[0]);

    pid = strtoul(argv[1], NULL, 10);

    if (!pid)
        return errorvalue(argv[1]);

    command_log(&remote, pid);

    return EXIT_SUCCESS;

}

static int parseremove(int argc, char **argv)
{

    struct remote remote;

    checkinit();

    if (remote_load(&remote, argv[0]))
        return errorremote(argv[0]);

    command_remove(&remote);

    return EXIT_SUCCESS;

}

static int parseshell(int argc, char **argv)
{

    struct remote remote;

    checkinit();

    if (remote_load(&remote, argv[0]))
        return errorremote(argv[0]);

    command_shell(&remote);

    return EXIT_SUCCESS;

}

static int parseshow(int argc, char **argv)
{

    struct remote remote;

    checkinit();

    if (remote_load(&remote, argv[0]))
        return errorremote(argv[0]);

    command_show(&remote);

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
        {"clone", parseclone, 2, " <name> <new-name>"},
        {"config", parseconfig, 3, " <name> <key> <value>"},
        {"exec", parseexec, 3, " <name> <num> <command>"},
        {"init", parseinit, 0, ""},
        {"list", parselist, 0, ""},
        {"log", parselog, 2, " <name> <pid>"},
        {"remove", parseremove, 1, " <name>"},
        {"shell", parseshell, 1, " <name>"},
        {"show", parseshow, 1, " <name>"},
        {"version", parseversion, 0, ""},
        {0}
    };

    return checkargs(commands, argc - 1, argv + 1);

}

