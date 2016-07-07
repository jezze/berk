#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "config.h"
#include "error.h"

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

