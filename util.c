#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"
#include "error.h"
#include "util.h"

int util_checkalnum(char *str)
{

    while (*str)
    {

        if (!isalnum(*str))
            return -1;

        str++;

    }

    return 0;

}

int util_checkalpha(char *str)
{

    while (*str)
    {

        if (!isalpha(*str))
            return -1;

        str++;

    }

    return 0;

}

int util_checkdigit(char *str)
{

    while (*str)
    {

        if (!isalpha(*str))
            return -1;

        str++;

    }

    return 0;

}

int util_checkspace(char *str)
{

    while (*str)
    {

        if (!isspace(*str))
            return -1;

        str++;

    }

    return 0;

}

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

char *util_nextword(char *str, unsigned int index, unsigned int total)
{

    if (!index)
        return str;

    if (index >= total)
        return NULL;

    while (*str)
        str++;

    return str + 1;

}

