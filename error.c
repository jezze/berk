#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"

void berk_panic(char *msg)
{

    fprintf(stderr, "berk: %s\n", msg);
    exit(EXIT_FAILURE);

}

