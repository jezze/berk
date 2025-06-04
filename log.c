#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "config.h"
#include "util.h"
#include "log.h"

#define LOG_ENTRYSIZE 77

int log_state_open(struct log_state *state)
{

    char path[BUFSIZ];

    config_get_subpath(path, BUFSIZ, CONFIG_LOGS, "HEAD");

    state->fd = open(path, O_RDONLY, 0644);

    if (state->fd < 0)
        return -1;

    state->size = lseek(state->fd, 0, SEEK_END);

    if (state->size % LOG_ENTRYSIZE != 0)
        return -1;

    state->position = state->size;

    return 0;

}

int log_state_close(struct log_state *state)
{

    close(state->fd);

    return 0;

}

int log_entry_prepare(struct log_entry *entry)
{

    char path[BUFSIZ];

    config_get_path(path, BUFSIZ, CONFIG_LOGS);

    if (util_mkdir(path) < 0)
        return -1;

    config_get_rundirshort(path, BUFSIZ, entry->id);

    if (util_mkdir(path) < 0)
        return -1;

    config_get_rundirfull(path, BUFSIZ, entry->id);

    if (util_mkdir(path) < 0)
        return -1;

    return 0;

}

int log_entry_read(struct log_entry *entry, struct log_state *state)
{

    char buffer[BUFSIZ];
    int count;

    lseek(state->fd, state->position, SEEK_SET);

    count = read(state->fd, buffer, LOG_ENTRYSIZE);

    if (count != LOG_ENTRYSIZE)
        return 0;

    sscanf(buffer, "%s %s %u %u %u %u\n", entry->id, entry->datetime, &entry->total, &entry->complete, &entry->passed, &entry->failed);

    return count;

}

int log_entry_readprev(struct log_entry *entry, struct log_state *state)
{

    if (state->position < LOG_ENTRYSIZE)
        return 0;

    state->position -= LOG_ENTRYSIZE;

    return log_entry_read(entry, state);

}

int log_entry_find(struct log_entry *entry, struct log_state *state, char *id)
{

    if (strcmp("HEAD", id) == 0)
        return log_entry_readprev(entry, state);

    while (log_entry_readprev(entry, state))
    {

        if (memcmp(entry->id, id, strlen(id)) == 0)
            return 1;

    }

    return 0;

}

int log_entry_printstd(struct log_entry *entry, unsigned int run, unsigned int descriptor)
{

    char buffer[BUFSIZ];
    unsigned int count;
    char path[BUFSIZ];
    int fd;

    switch (descriptor)
    {

    case 1:
        config_get_runpath(path, BUFSIZ, entry->id, run, "stdout");

        break;

    case 2:
        config_get_runpath(path, BUFSIZ, entry->id, run, "stderr");

        break;

    }

    fd = open(path, O_RDONLY, 0644);

    if (fd < 0)
        return -1;

    while ((count = read(fd, buffer, BUFSIZ)))
        write(STDOUT_FILENO, buffer, count);

    close(fd);

    return 0;

}

int log_entry_printrun(struct log_entry *entry, unsigned int run)
{

    char status[BUFSIZ];
    char remote[BUFSIZ];
    unsigned int count;
    char path[BUFSIZ];
    int fd;

    config_get_runpath(path, BUFSIZ, entry->id, run, "remote");

    fd = open(path, O_RDONLY, 0644);

    if (fd < 0)
        return -1;

    count = read(fd, remote, BUFSIZ);
    remote[count - 1] = '\0';

    close(fd);

    config_get_runpath(path, BUFSIZ, entry->id, run, "status");

    fd = open(path, O_RDONLY, 0644);

    if (fd < 0)
        return -1;

    count = read(fd, status, BUFSIZ);
    status[count - 1] = '\0';

    close(fd);

    printf("run=%u remote=%s status=%s\n", run, remote, status);

    return 0;

}

int log_entry_print(struct log_entry *entry)
{

    char path[BUFSIZ];
    unsigned int i;

    config_get_rundirfull(path, BUFSIZ, entry->id);
    printf("id=%s datetime=%s\n", entry->id, entry->datetime);
    printf("total=%u complete=%u passed=%u failed=%u\n", entry->total, entry->complete, entry->passed, entry->failed);
    printf("\n");

    for (i = 0; i < entry->total; i++)
    {

        printf("    ");
        log_entry_printrun(entry, i);

    }

    printf("\n");

    return 0;

}

int log_entry_write(struct log_entry *entry)
{

    char path[BUFSIZ];
    time_t timeraw;
    int fd;

    time(&timeraw);
    strftime(entry->datetime, 25, "%FT%T%z", localtime(&timeraw));
    config_get_subpath(path, BUFSIZ, CONFIG_LOGS, "HEAD");

    fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (fd < 0)
        return -1;

    if (dprintf(fd, "%s %s %04d %04d %04d %04d\n", entry->id, entry->datetime, entry->total, entry->complete, entry->passed, entry->failed) != LOG_ENTRYSIZE)
        return -1;

    close(fd);

    return 0;

}

static void createid(char *dest, unsigned int length)
{

    char charset[] = "0123456789abcdef";
    unsigned int i;

    srand(time(NULL));

    for (i = 0; i < length; i++)
    {

        unsigned int index = (double)rand() / RAND_MAX * 16;

        dest[i] = charset[index];

    }

    dest[length - 1] = '\0';

}

void log_entry_init(struct log_entry *entry)
{

    memset(entry, 0, sizeof (struct log_entry));
    createid(entry->id, 32);

    entry->total = 0;
    entry->complete = 0;
    entry->passed = 0;
    entry->failed = 0;

}

