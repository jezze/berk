#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "berk.h"
#include "init.h"
#include "job.h"
#include "remote.h"
#include "version.h"

int berk_error_command(char *name, struct berk_command *cmds)
{

    unsigned int i;

    if (name)
        fprintf(stderr, "Usage: berk %s <command> [<args>]\n\n", name);
    else
        fprintf(stderr, "Usage: berk <command> [<args>]\n\n");

    fprintf(stderr, "List of commands:\n");

    for (i = 0; cmds[i].name; i++)
    {

        if (cmds[i].description)
            fprintf(stderr, "    %s %s\n", cmds[i].name, cmds[i].description);
        else
            fprintf(stderr, "    %s\n", cmds[i].name);

    }

    return EXIT_FAILURE;

}

int berk_error_extra()
{

    fprintf(stderr, "berk: Too many arguments\n");

    return EXIT_FAILURE;

}

int berk_error_invalid(char *arg)
{

    fprintf(stderr, "berk: Invalid argument '%s'\n", arg);

    return EXIT_FAILURE;

}

int berk_error_missing()
{

    fprintf(stderr, "berk: Missing argument\n");

    return EXIT_FAILURE;

}

int berk_error(char *msg)
{

    fprintf(stderr, "berk: %s\n", msg);

    return EXIT_FAILURE;

}

void berk_panic(char *msg)
{

    fprintf(stderr, "berk: %s\n", msg);
    exit(EXIT_FAILURE);

}

int main(int argc, char **argv)
{

    static struct berk_command cmds[] = {
        {"init", init_parse, 0},
        {"job", job_parse, "[...]"},
        {"remote", remote_parse, "[...]"},
        {"version", version_parse, 0},
        {0}
    };
    unsigned int i;

    if (argc < 2)
        return berk_error_command(0, cmds);

    for (i = 0; cmds[i].name; i++)
    {

        if (strcmp(argv[1], cmds[i].name))
            continue;

        return cmds[i].parse(argc - 2, argv + 2);

    }

    return berk_error_invalid(argv[1]);

}

