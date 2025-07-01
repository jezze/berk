#include <stdlib.h>
#include "util.h"
#include "args.h"

int args_next(struct args_state *state, int argc, char **argv)
{

    state->type = ARGS_TYPE_NONE;
    state->flag = 0;
    state->hash = 0;
    state->arg = NULL;

    if (state->argi >= argc)
        return 0;

    state->arg = argv[state->argi];

    if (state->arg[0] == '-')
    {

        state->type = ARGS_TYPE_FLAG;
        state->flag = state->arg[1];
        state->hash = util_hash(state->arg);

    }

    else
    {

        state->type = ARGS_TYPE_COMMAND;

    }

    return ++state->argi;

}

unsigned int args_position(struct args_state *state)
{

    return state->argp++;

}

void args_init(struct args_state *state)
{

    state->argp = 0;
    state->argi = 0;
    state->type = ARGS_TYPE_NONE;
    state->flag = 0;
    state->hash = 0;
    state->arg = NULL;

}

