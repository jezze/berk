#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "config.h"

int error(char *format, ...)
{

    va_list args;

    va_start(args, format);
    fprintf(stderr, "%s: ", CONFIG_PROGNAME);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);

    return EXIT_FAILURE;

}

int error_init(void)
{

    return error("Could not find '%s' directory.", CONFIG_ROOT);

}

int error_remote_prepare(char *name)
{

    return error("Could not init remote '%s'.", name);

}

int error_remote_load(char *name)
{

    return error("Could not load remote '%s'.", name);

}

int error_remote_save(char *name)
{

    return error("Could not save remote '%s'.", name);

}

int error_remote_connect(char *name)
{

    return error("Could not connect to remote '%s'.", name);

}

int error_remote_disconnect(char *name)
{

    return error("Could not disconnect from remote '%s'.", name);

}

int error_run_prepare(unsigned int index)
{

    return error("Could not prepare run '%u'.", index);

}

int error_run_update(unsigned int index, char *type)
{

    return error("Could not update run '%u' with type '%s'.", index, type);

}

int error_run_open(unsigned int index)
{

    return error("Could not open run '%u'.", index);

}

int error_run_close(unsigned int index)
{

    return error("Could not close run '%u'.", index);

}

int error_missing(void)
{

    return error("Missing arguments.");

}

int error_toomany(void)
{

    return error("Too many arguments.");

}

int error_flag_unrecognized(char *arg)
{

    return error("Unrecognized flag '%s'.", arg);

}

int error_arg_invalid(char *arg)
{

    return error("Invalid argument '%s'.", arg);

}

int error_arg_parse(char *arg, char *type)
{

    return error("Could not parse '%s' as '%s'.", arg, type);

}

