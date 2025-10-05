#include <stdlib.h>
#include <string.h>
#include "args.h"

static unsigned int getstate(struct args *args)
{

    if (args->value[0] == '-')
    {

        if (args->options)
        {

            unsigned int length = strlen(args->options);
            unsigned int i;

            for (i = 0; i < length; i++)
            {

                if (args->options[i] != args->value[1])
                    continue;

                switch (args->options[i + 1])
                {

                case ':':
                    return ARGS_OPTION;

                default:
                    return ARGS_FLAG;

                }

            }

        }

        return ARGS_ERROR;

    }

    return ARGS_COMMAND;

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

    if (args->index >= args->argc)
    {

        args->state = ARGS_DONE;

        return 0;

    }

    args->value = args->argv[args->index];
    args->state = getstate(args);
    args->index++;

    switch (args->state)
    {

    case ARGS_COMMAND:
        args->flag = 0xFF;
        args->position++;

        break;

    case ARGS_FLAG:
        args->flag = args->value[1];

        break;

    case ARGS_OPTION:
        args->flag = args->value[1];

        if (args->index >= args->argc)
            return 0;

        args->value = args->argv[args->index];
        args->index++;

        break;

    case ARGS_ERROR:
    default:
        args->flag = 0;

        break;

    }

    return args->flag;

}

