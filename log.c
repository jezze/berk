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

int log_prepare(char *id)
{

    char path[BUFSIZ];

    if (config_get_path(path, BUFSIZ, CONFIG_LOGS))
        return -1;

    if (access(path, F_OK) && mkdir(path, 0775) < 0)
        return -1;

    if (config_get_shortrun(path, BUFSIZ, id))
        return -1;

    if (access(path, F_OK) && mkdir(path, 0775) < 0)
        return -1;

    if (config_get_fullrun(path, BUFSIZ, id))
        return -1;

    if (access(path, F_OK) && mkdir(path, 0775) < 0)
        return -1;

    return 0;

}

int log_open_head(struct log_state *state)
{

    char path[BUFSIZ];

    if (config_get_path(path, BUFSIZ, CONFIG_LOGS "/HEAD"))
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

void log_close_head(struct log_state *state)
{

    fclose(state->file);

}

long log_previous(struct log_state *state)
{

    return state->position -= LOG_ENTRYSIZE;

}

int log_readentry(struct log_state *state, struct log_entry *entry)
{

    fseek(state->file, state->position, SEEK_SET);

    return fscanf(state->file, "%s %s %u %u %u\n", entry->id, entry->datetime, &entry->total, &entry->complete, &entry->success);

}

int log_find(struct log_state *state, struct log_entry *entry, char *id)
{

    while (log_previous(state) >= 0)
    {

        int result = log_readentry(state, entry);

        if (result == 5 && memcmp(entry->id, id, strlen(id)) == 0)
            return 1;

    }

    return 0;

}

int log_printentry(struct log_entry *entry)
{

    char path[BUFSIZ];
    unsigned int i;

    if (config_get_fullrun(path, BUFSIZ, entry->id))
        return -1;

    printf("id=%s datetime=%s\n", entry->id, entry->datetime);
    printf("total=%u complete=%u successful=%u failed=%u\n", entry->total, entry->complete, entry->success, entry->total - entry->success);
    printf("\n");

    for (i = 0; i < entry->total; i++)
        printf("    run=%u remote=xxx status=successful\n", i);

    printf("\n");

    return 0;

}

int log_write(int fd, char *buffer, unsigned int size)
{

    return write(fd, buffer, size);

}

int log_write_head(char *id, int total, int complete, int success)
{

    char path[BUFSIZ];
    char buffer[BUFSIZ];
    char datetime[64];
    unsigned int count;
    int fd;
    time_t timeraw;
    struct tm *timeinfo;

    time(&timeraw);

    timeinfo = localtime(&timeraw);

    strftime(datetime, 64, "%FT%T%z", timeinfo);

    count = snprintf(buffer, BUFSIZ, "%s %s %04d %04d %04d\n", id, datetime, total, complete, success);

    if (config_get_path(path, BUFSIZ, CONFIG_LOGS "/HEAD"))
        return -1;

    fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (fd < 0)
        return -1;

    write(fd, buffer, count);
    close(fd);

    return 0;

}

