#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"
#include "error.h"
#include "util.h"

void util_trim(char *a)
{

    char *p = a, *q = a;

    while (isspace(*q))
        ++q;

    while (*q)
        *p++ = *q++;

    *p = '\0';

    while (p > a && isspace(*--p))
        *p = '\0';

}

unsigned int util_seperatewords(char *buffer)
{

    unsigned int total = 0;
    unsigned int blank = 0;

    while (*buffer != '\0')
    {

        if (isspace(*buffer))
        {

            *buffer = '\0';

            if (!blank)
                total++;

            blank = 1;

        }

        else
        {

            blank = 0;

        }

        buffer++;

    }

    return total + 1;

}

char *util_nextword(char *buffer, unsigned int index, unsigned int words)
{

    if (!index)
        return buffer;

    if (index >= words)
        return NULL;

    while (*buffer != '\0')
        buffer++;

    while (*buffer == '\0')
        buffer++;

    return buffer;

}

