#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
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

static char *lskip(char *s)
{

    while (*s && isspace((unsigned char)(*s)))
        s++;

    return (char *)s;

}

static char *find_char_or_comment(char *s, char c)
{

    int was_whitespace = 0;

    while (*s && *s != c && !(was_whitespace && *s == ';'))
    {

        was_whitespace = isspace((unsigned char)(*s));

        s++;

    }

    return (char *)s;

}

static char *strncpy0(char *dest, char *src, size_t size)
{

    strncpy(dest, src, size);

    dest[size - 1] = '\0';

    return dest;

}

static unsigned int findnl(char *buffer, unsigned int count)
{

    unsigned int i;

    for (i = 0; i < count; i++)
    {

        if (buffer[i] == '\n')
            return i;

    }

    return 0;

}

static int ini_parse_file(int fd, int (*handler)(void *user, char *section, char *key, char *value), void *user)
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
    unsigned int offset = 0;

    while (read(fd, line, MAX_LINE) > 0)
    {

        unsigned int c = findnl(line, MAX_LINE);

        line[c] = '\0';
        offset += c + 1;
        lseek(fd, offset, SEEK_SET);

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

int ini_parse(char *filename, int (*handler)(void *user, char *section, char *name, char *value), void *user)
{

    int error;
    int fd;

    fd = open(filename, O_RDONLY, 0644);

    if (fd < 0)
        return -1;

    error = ini_parse_file(fd, handler, user);

    close(fd);

    return error;

}

int ini_write_section(int fd, char *name)
{

    return dprintf(fd, "[%s]\n", name);

}

int ini_write_int(int fd, char *key, int value)
{

    return dprintf(fd, "        %s = %d\n", key, value);

}

int ini_write_string(int fd, char *key, char *value)
{

    return dprintf(fd, "        %s = %s\n", key, value);

}

