#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "config.h"
#include "util.h"
#include "log.h"

#define LOG_ENTRYSIZE 82

int log_open(struct log *log)
{

    char path[BUFSIZ];

    config_get_subpath(path, BUFSIZ, CONFIG_LOGS, "HEAD");

    log->fd = open(path, O_RDONLY, 0644);

    if (log->fd >= 0)
    {

        flock(log->fd, LOCK_SH);

        log->size = lseek(log->fd, 0, SEEK_END);

        flock(log->fd, LOCK_UN);

        if (log->size % LOG_ENTRYSIZE != 0)
            return -1;

        log->position = log->size;

        return 0;

    }

    return -1;

}

int log_close(struct log *log)
{

    close(log->fd);

    return 0;

}

int log_prepare(struct log *log)
{

    char path[BUFSIZ];

    config_get_path(path, BUFSIZ, CONFIG_LOGS);

    if (util_mkdir(path) < 0)
        return -1;

    config_get_rundirshort(path, BUFSIZ, log->id);

    if (util_mkdir(path) < 0)
        return -1;

    config_get_rundirfull(path, BUFSIZ, log->id);

    if (util_mkdir(path) < 0)
        return -1;

    return 0;

}

int log_read(struct log *log)
{

    char buffer[BUFSIZ];
    int count;

    flock(log->fd, LOCK_SH);
    lseek(log->fd, log->position, SEEK_SET);

    count = read(log->fd, buffer, LOG_ENTRYSIZE);

    flock(log->fd, LOCK_UN);

    if (count < 0)
        return -1;

    if (count != LOG_ENTRYSIZE)
        return -1;

    sscanf(buffer, "%s %s %u %u %u %u %u\n", log->id, log->datetime, &log->total, &log->complete, &log->aborted, &log->passed, &log->failed);

    return count;

}

int log_moveprev(struct log *log, unsigned int steps)
{

    unsigned int bytes = steps * LOG_ENTRYSIZE;

    if (log->position >= bytes)
    {

        log->position -= bytes;

        return log->position;

    }

    return -1;

}

int log_find(struct log *log, char *id)
{

    unsigned int length = strlen(id);

    if (length > 32)
        return 0;

    if (length == 4 && memcmp(id, "HEAD", 4) == 0)
    {

        if (log_moveprev(log, 1) >= 0)
            return log_read(log);

    }

    if (length > 5 && memcmp(id, "HEAD~", 5) == 0)
    {

        unsigned int count = 0;

        sscanf(id, "HEAD~%u", &count);

        if (log_moveprev(log, count + 1) >= 0)
            return log_read(log);

    }

    while (log_moveprev(log, 1) >= 0)
    {

        if (log_read(log) < 0)
            return -1;

        if (memcmp(log->id, id, length) == 0)
            return LOG_ENTRYSIZE;

    }

    return 0;

}

void log_print(struct log *log)
{

    printf("id=%s datetime=%s\n", log->id, log->datetime);
    printf("total=%u complete=%u aborted=%u passed=%u failed=%u\n", log->total, log->complete, log->aborted, log->passed, log->failed);

}

int log_add(struct log *log)
{

    char path[BUFSIZ];
    time_t timeraw;
    int fd;

    time(&timeraw);
    strftime(log->datetime, 25, "%FT%T%z", localtime(&timeraw));
    config_get_subpath(path, BUFSIZ, CONFIG_LOGS, "HEAD");

    fd = open(path, O_WRONLY | O_CREAT | O_SYNC, 0644);

    if (fd >= 0)
    {

        int count;

        flock(fd, LOCK_SH);

        log->position = lseek(fd, 0, SEEK_END);

        count = dprintf(fd, "%s %s %04d %04d %04d %04d %04d\n", log->id, log->datetime, log->total, log->complete, log->aborted, log->passed, log->failed);

        flock(fd, LOCK_UN);
        close(fd);

        return (count == LOG_ENTRYSIZE) ? 0 : -1;

    }

    return -1;

}

int log_update(struct log *log)
{

    char path[BUFSIZ];
    int fd;

    config_get_subpath(path, BUFSIZ, CONFIG_LOGS, "HEAD");

    fd = open(path, O_WRONLY | O_SYNC, 0644);

    if (fd >= 0)
    {

        int count;

        flock(fd, LOCK_SH);
        lseek(fd, log->position, SEEK_SET);

        count = dprintf(fd, "%s %s %04d %04d %04d %04d %04d\n", log->id, log->datetime, log->total, log->complete, log->aborted, log->passed, log->failed);

        flock(fd, LOCK_UN);
        close(fd);

        return (count == LOG_ENTRYSIZE) ? 0 : -1;

    }

    return 0;

}

static void createid(char *dest, unsigned int length)
{

    char charset[] = "0123456789abcdef";
    unsigned int i;
    struct timeval t1;

    gettimeofday(&t1, NULL);
    srand(t1.tv_usec * t1.tv_sec);

    for (i = 0; i < length; i++)
    {

        unsigned int index = (double)rand() / RAND_MAX * 16;

        dest[i] = charset[index];

    }

    dest[length - 1] = '\0';

}

void log_init(struct log *log, unsigned int total)
{

    memset(log, 0, sizeof (struct log));
    createid(log->id, 32);

    log->total = total;
    log->complete = 0;
    log->aborted = 0;
    log->passed = 0;
    log->failed = 0;
    log->fd = 0;
    log->size = 0;
    log->position = 0;

}

