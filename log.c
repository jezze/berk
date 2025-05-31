#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "config.h"
#include "log.h"

#define LOG_ENTRYSIZE 72

int log_state_open(struct log_state *state)
{

    char path[BUFSIZ];

    if (config_get_subpath(path, BUFSIZ, CONFIG_LOGS, "HEAD"))
        return -1;

    state->file = fopen(path, "r");

    if (state->file == NULL)
        return 0;

    fseek(state->file, 0, SEEK_END);

    state->size = ftell(state->file);

    if (state->size % LOG_ENTRYSIZE != 0)
        return -1;

    state->position = state->size;

    return 0;

}

int log_state_close(struct log_state *state)
{

    fclose(state->file);

    return 0;

}

int log_entry_prepare(struct log_entry *entry)
{

    char path[BUFSIZ];

    if (config_get_path(path, BUFSIZ, CONFIG_LOGS))
        return -1;

    if (access(path, F_OK) && mkdir(path, 0775) < 0)
        return -1;

    if (config_get_rundirshort(path, BUFSIZ, entry->id))
        return -1;

    if (access(path, F_OK) && mkdir(path, 0775) < 0)
        return -1;

    if (config_get_rundirfull(path, BUFSIZ, entry->id))
        return -1;

    if (access(path, F_OK) && mkdir(path, 0775) < 0)
        return -1;

    return 0;

}

int log_entry_read(struct log_entry *entry, struct log_state *state)
{

    fseek(state->file, state->position, SEEK_SET);

    return fscanf(state->file, "%s %s %u %u %u\n", entry->id, entry->datetime, &entry->total, &entry->complete, &entry->success);

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

int log_entry_print(struct log_entry *entry)
{

    char path[BUFSIZ];
    unsigned int i;

    if (config_get_rundirfull(path, BUFSIZ, entry->id))
        return -1;

    printf("id=%s datetime=%s\n", entry->id, entry->datetime);
    printf("total=%u complete=%u successful=%u failed=%u\n", entry->total, entry->complete, entry->success, entry->total - entry->success);
    printf("\n");

    for (i = 0; i < entry->total; i++)
        printf("    run=%u remote=xxx status=successful\n", i);

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

    if (config_get_subpath(path, BUFSIZ, CONFIG_LOGS, "HEAD"))
        return -1;

    fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (fd < 0)
        return -1;

    if (dprintf(fd, "%s %s %04d %04d %04d\n", entry->id, entry->datetime, entry->total, entry->complete, entry->success) != LOG_ENTRYSIZE)
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
    entry->success = 0;

}

