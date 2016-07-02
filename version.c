#include <stdio.h>
#include <stdlib.h>
#include "berk.h"

int version_parse(int argc, char **argv)
{

    if (argc)
        return berk_error_extra();

    fprintf(stdout, "berk version %s\n", BERK_VERSION);

    return EXIT_SUCCESS;

}

