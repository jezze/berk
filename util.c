#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"
#include "error.h"
#include "util.h"

void util_trim(char *str)
{

    char *dest = str;

    while (isspace(*str))
        str++;

    while (*str)
        *dest++ = *str++;

    *dest = '\0';

    while (isspace(*--dest))
        *dest = '\0';

}

void util_strip(char *str)
{

    char *dest = str;

    while (*str)
    {

        while (isspace(*str) && isspace(*(str + 1)))
            str++;

       *dest++ = *str++;

    }

    *dest = '\0';

}

unsigned int util_split(char *str)
{

    unsigned int total = 1;

    while (*str)
    {

        if (isspace(*str))
        {

            *str = '\0';
            total++;

        }

        str++;

    }

    return total;

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

