#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "error.h"
#include "init.h"
#include "job.h"
#include "remote.h"
#include "run.h"

struct command
{

    char *name;
    int (*parse)(int argc, char **argv);
    int argc;
    char *description;

};

static int errorresource(char *name)
{

    error(ERROR_NORMAL, "Could not find resource '%s'.", name);

    return EXIT_FAILURE;

}

static int errorarguments()
{

    error(ERROR_NORMAL, "Too many arguments.");

    return EXIT_FAILURE;

}

static int checkargs(char *name, struct command *commands, int argc, char **argv)
{

    unsigned int i;

    if (!argc)
    {

        if (name)
            fprintf(stdout, "Usage: %s %s <command> [<args>]\n\n", BERK_NAME, name);
        else
            fprintf(stdout, "Usage: %s <command> [<args>]\n\n", BERK_NAME);

        fprintf(stdout, "List of commands:\n");

        for (i = 0; commands[i].name; i++)
        {

            if (commands[i].description)
                fprintf(stdout, "    %s %s\n", commands[i].name, commands[i].description);
            else
                fprintf(stdout, "    %s\n", commands[i].name);

        }

        return EXIT_SUCCESS;

    }

    for (i = 0; commands[i].name; i++)
    {

        if (strcmp(argv[0], commands[i].name))
            continue;

        if ((argc - 1) < commands[i].argc)
        {

            if (commands[i].description)
                fprintf(stdout, "Usage: %s %s %s %s\n", BERK_NAME, name, commands[i].name, commands[i].description);
            else
                fprintf(stdout, "Usage: %s %s %s\n", BERK_NAME, name, commands[i].name);

            return EXIT_SUCCESS;

        }

        return commands[i].parse(argc - 1, argv + 1);

    }

    error(ERROR_NORMAL, "Invalid argument '%s'.", argv[0]);

    return EXIT_FAILURE;

}

static int parseinit(int argc, char **argv)
{

    if (argc)
        return errorarguments();

    init_setup();

    return EXIT_SUCCESS;

}

static int parsejobcopy(int argc, char **argv)
{

    struct job job;

    init_assert();

    if (job_load(argv[0], &job))
        return errorresource(argv[0]);

    job_copy(&job, argv[1]);

    return EXIT_SUCCESS;

}

static int parsejobcreate(int argc, char **argv)
{

    init_assert();
    job_create(argv[0]);

    return EXIT_SUCCESS;

}

static int parsejoblist(int argc, char **argv)
{

    if (argc)
        return errorarguments();

    job_list();

    return EXIT_SUCCESS;

}

static int parsejobremove(int argc, char **argv)
{

    struct job job;

    init_assert();

    if (job_load(argv[0], &job))
        return errorresource(argv[0]);

    job_remove(&job);

    return EXIT_SUCCESS;

}

static int parsejobshow(int argc, char **argv)
{

    struct job job;

    init_assert();

    if (job_load(argv[0], &job))
        return errorresource(argv[0]);

    job_show(&job);

    return EXIT_SUCCESS;

}

static int parsejob(int argc, char **argv)
{

    static struct command commands[] = {
        {"list", parsejoblist, 0, 0},
        {"show", parsejobshow, 1, "<id>"},
        {"create", parsejobcreate, 1, "<id>"},
        {"remove", parsejobremove, 1, "<id>"},
        {"copy", parsejobcopy, 2, "<id> <new-id>"},
        {0}
    };

    return checkargs("job", commands, argc, argv);

}

static int parseremotecopy(int argc, char **argv)
{

    struct remote remote;

    init_assert();

    if (remote_load(argv[0], &remote))
        return errorresource(argv[0]);

    remote_copy(&remote, argv[1]);

    return EXIT_SUCCESS;

}

static int parseremotecreate(int argc, char **argv)
{

    init_assert();
    remote_create(argv[0], argv[1], getenv("USER"));

    return EXIT_SUCCESS;

}

static int parseremotelist(int argc, char **argv)
{

    if (argc)
        return errorarguments();

    remote_list();

    return EXIT_SUCCESS;

}

static int parseremoteremove(int argc, char **argv)
{

    struct remote remote;

    init_assert();

    if (remote_load(argv[0], &remote))
        return errorresource(argv[0]);

    remote_remove(&remote);

    return EXIT_SUCCESS;

}

static int parseremoteshow(int argc, char **argv)
{

    struct remote remote;

    init_assert();

    if (remote_load(argv[0], &remote))
        return errorresource(argv[0]);

    remote_show(&remote);

    return EXIT_SUCCESS;

}

static int parseremote(int argc, char **argv)
{

    static struct command commands[] = {
        {"list", parseremotelist, 0, 0},
        {"show", parseremoteshow, 1, "<id>"},
        {"create", parseremotecreate, 2, "<id> <hostname>"},
        {"remove", parseremoteremove, 1, "<id>"},
        {"copy", parseremotecopy, 2, "<id> <new-id>"},
        {0}
    };

    return checkargs("remote", commands, argc, argv);

}

static int parserun(int argc, char **argv)
{

    struct job job;
    struct remote remote;

    init_assert();

    if (job_load(argv[0], &job))
        return errorresource(argv[0]);

    if (remote_load(argv[1], &remote))
        return errorresource(argv[1]);

    run(&job, &remote);

    return EXIT_SUCCESS;

}

static int parseversion(int argc, char **argv)
{

    if (argc)
        return errorarguments();

    fprintf(stdout, "%s: version %s\n", BERK_NAME, BERK_VERSION);

    return EXIT_SUCCESS;

}

int main(int argc, char **argv)
{

    static struct command commands[] = {
        {"init", parseinit, 0, 0},
        {"job", parsejob, 0, "[...]"},
        {"remote", parseremote, 0, "[...]"},
        {"run", parserun, 2, "<job-id> <remote-id>"},
        {"version", parseversion, 0, 0},
        {0}
    };

    return checkargs(NULL, commands, argc - 1, argv + 1);

}

