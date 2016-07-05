#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "ini.h"

#define MAX_LINE 1024
#define MAX_SECTION 32
#define MAX_NAME 32

static char *rstrip(char *s)
{
    char *p = s + strlen(s);

    while (p > s && isspace((unsigned char)(*--p)))
        *p = '\0';

    return s;

}

static char *lskip(const char *s)
{

    while (*s && isspace((unsigned char)(*s)))
        s++;

    return (char *)s;

}

static char *find_char_or_comment(const char *s, char c)
{

    int was_whitespace = 0;

    while (*s && *s != c && !(was_whitespace && *s == ';'))
    {

        was_whitespace = isspace((unsigned char)(*s));

        s++;

    }

    return (char *)s;

}

static char *strncpy0(char *dest, const char *src, size_t size)
{

    strncpy(dest, src, size);

    dest[size - 1] = '\0';

    return dest;

}

static int ini_parse_file(FILE *file, int (*handler)(void *user, const char *section, const char *name, const char *value), void *user)
{

    char line[MAX_LINE];
    char section[MAX_SECTION] = "";
    char prev_name[MAX_NAME] = "";
    char *start;
    char *end;
    char *name;
    char *value;
    int lineno = 0;
    int error = 0;

    while (fgets(line, MAX_LINE, file) != NULL)
    {

        lineno++;
        start = line;
        start = lskip(rstrip(start));

        if (*start == ';' || *start == '#')
        {

        }

        else if (*start == '[')
        {

            end = find_char_or_comment(start + 1, ']');

            if (*end == ']')
            {

                *end = '\0';

                strncpy0(section, start + 1, sizeof (section));

                *prev_name = '\0';

            }

            else if (!error)
            {

                error = lineno;

            }

        }

        else if (*start && *start != ';')
        {

            end = find_char_or_comment(start, '=');

            if (*end != '=')
            {

                end = find_char_or_comment(start, ':');

            }

            if (*end == '=' || *end == ':')
            {

                *end = '\0';
                name = rstrip(start);
                value = lskip(end + 1);
                end = find_char_or_comment(value, '\0');

                if (*end == ';')
                    *end = '\0';

                rstrip(value);
                strncpy0(prev_name, name, sizeof (prev_name));

                if (handler(user, section, name, value) && !error)
                    error = lineno;

            }

            else if (!error)
            {

                error = lineno;

            }

        }

        if (error)
            break;

    }

    return error;

}

int ini_parse(const char *filename, int (*handler)(void *user, const char *section, const char *name, const char *value), void *user)
{

    FILE *file;
    int error;

    file = fopen(filename, "r");

    if (!file)
        return -1;

    error = ini_parse_file(file, handler, user);

    fclose(file);

    return error;

}

int ini_writesection(FILE *file, char *name)
{

    return fprintf(file, "[%s]\n", name);

}

int ini_writeint(FILE *file, char *key, int value)
{

    return fprintf(file, "        %s = %d\n", key, value);

}

int ini_writestring(FILE *file, char *key, char *value)
{

    return fprintf(file, "        %s = %s\n", key, value);

}

