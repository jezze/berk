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

static int parseadd(int argc, char **argv)
{

    config_init();
    command_add(argv[0], argv[1], getenv("USER"));

    return EXIT_SUCCESS;

}

static int parseconfig(int argc, char **argv)
{

    struct remote remote;

    config_init();

    if (remote_load(&remote, argv[0]))
        return errorremote(argv[0]);

    command_config(&remote, argv[1], argv[2]);

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

    unsigned int total = 0;
    unsigned int complete = 0;
    unsigned int success = 0;
    unsigned int words;
    unsigned int i;
    char *word = argv[0];
    int status;

    config_init();
    util_trim(word);
    util_strip(word);

    words = util_split(word);

    if (event_begin())
        error(ERROR_PANIC, "Could not run event.");

    for (i = 0; (word = util_nextword(word, i, words)); i++)
    {

        pid_t pid = fork();

        if (pid == 0)
        {

            struct remote remote;

            if (remote_load(&remote, word))
                return errorremote(word);

            return command_exec(&remote, getpid(), argv[1]);

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

    config_init();

    if (argc > 0)
        command_list(argv[0]);
    else
        command_list(NULL);

    return EXIT_SUCCESS;

}

static int parselog(int argc, char **argv)
{

    struct remote remote;
    unsigned int pid;

    config_init();

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

    config_init();

    if (remote_load(&remote, argv[0]))
        return errorremote(argv[0]);

    command_remove(&remote);

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

    struct remote remote;

    config_init();

    if (remote_load(&remote, argv[0]))
        return errorremote(argv[0]);

    command_shell(&remote);

    return EXIT_SUCCESS;

}

static int parseshow(int argc, char **argv)
{

    struct remote remote;

    config_init();

    if (remote_load(&remote, argv[0]))
        return errorremote(argv[0]);

    if (argc > 1)
        command_show(&remote, argv[1]);
    else
        command_show(&remote, NULL);
    
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
        {"config", parseconfig, 3, " <name> <key> <value>"},
        {"copy", parsecopy, 2, " <name>:<path> <name>:<path>"},
        {"exec", parseexec, 2, " <namelist> <command>"},
        {"init", parseinit, 0, ""},
        {"list", parselist, 0, " [<label>]"},
        {"log", parselog, 2, " <name> <pid>"},
        {"remove", parseremove, 1, " <name>"},
        {"send", parsesend, 2, " <namelist> <path>"},
        {"shell", parseshell, 1, " <name>"},
        {"show", parseshow, 1, " <name> [<key>]"},
        {"version", parseversion, 0, ""},
        {0}
    };

    return checkargs(commands, argc - 1, argv + 1);

}

