#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "error.h"

void berk_panic(char *msg)
{

    fprintf(stderr, "%s: %s\n", BERK_NAME, msg);
    exit(EXIT_FAILURE);

}

