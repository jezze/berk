#include <stdlib.h>
#include <string.h>
#include "args.h"

static unsigned int gettype(struct args *args, char flag)
{

    unsigned int length;
    unsigned int i;

    if (!args->options)
        return 0;

    length = strlen(args->options);

    for (i = 0; i < length; i++)
    {

        if (args->options[i] != flag)
            continue;

        switch (args->options[i + 1])
        {

        case ':':
            return ARGS_OPTION;

        default:
            return ARGS_FLAG;

        }

    }

    return ARGS_ERROR;

}

void args_setoptions(struct args *args, char *options)
{

    args->options = options;
    args->position = 0;

}

void args_init(struct args *args, int argc, char **argv)
{

    args->index = 0;
    args->position = 0;
    args->state = ARGS_NONE;
    args->flag = 0;
    args->options = 0;
    args->value = 0;
    args->argc = argc;
    args->argv = argv;

}

unsigned int args_next(struct args *args)
{

    if (args->state == ARGS_ERROR)
        return 0;

    if (args->index >= args->argc)
    {

        args->state = ARGS_DONE;

        return 0;

    }

    args->state = ARGS_NONE;
    args->value = args->argv[args->index];
    args->flag = 0;
    args->index++;

    if (args->value[0] == '-')
    {

        args->state = gettype(args, args->value[1]);
        args->flag = args->value[1];

        switch (args->state)
        {

        case ARGS_ERROR:
            return 0;

        case ARGS_FLAG:
            break;

        case ARGS_OPTION:
            args->value = args->argv[args->index];
            args->index++;

            break;

        default:
            return 0;

        }

    }

    else
    {

        args->state = ARGS_COMMAND;
        args->position++;
        args->flag = 0xFF;

    }

    return args->flag;

}

