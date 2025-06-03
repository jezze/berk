#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include "util.h"

char *util_assert_alnum(char *str)
{

    while (*str)
    {

        if (!isalnum(*str))
            return 0;

        str++;

    }

    return str;

}

char *util_assert_alpha(char *str)
{

    while (*str)
    {

        if (!isalpha(*str))
            return 0;

        str++;

    }

    return str;

}

char *util_assert_digit(char *str)
{

    while (*str)
    {

        if (!isdigit(*str))
            return 0;

        str++;

    }

    return str;

}

char *util_assert_xdigit(char *str)
{

    while (*str)
    {

        if (!isxdigit(*str))
            return 0;

        str++;

    }

    return str;

}

char *util_assert_print(char *str)
{

    while (*str)
    {

        if (!isprint(*str))
            return 0;

        str++;

    }

    return str;

}

char *util_assert_space(char *str)
{

    while (*str)
    {

        if (!isspace(*str))
            return 0;

        str++;

    }

    return str;

}

char *util_assert_printspace(char *str)
{

    while (*str)
    {

        if (!isprint(*str) && !isspace(*str))
            return 0;

        str++;

    }

    return str;

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

int util_mkdir(char *path)
{

    if (access(path, F_OK) && mkdir(path, 0775) < 0)
        return -1;

    return 0;

}

int util_unlink(char *path)
{

    if (unlink(path) < 0)
        return -1;

    return 0;

}

unsigned int util_hash(char *str)
{

    unsigned int hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;

    return hash;

}

