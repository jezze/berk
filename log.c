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

    if (log->fd < 0)
        return -1;

    flock(log->fd, LOCK_SH);

    log->size = lseek(log->fd, 0, SEEK_END);

    flock(log->fd, LOCK_UN);

    if (log->size % LOG_ENTRYSIZE != 0)
        return -1;

    log->position = log->size;

    return 0;

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

    if (count != LOG_ENTRYSIZE)
        return 0;

    sscanf(buffer, "%s %s %u %u %u %u %u\n", log->id, log->datetime, &log->total, &log->complete, &log->aborted, &log->passed, &log->failed);

    return count;

}

int log_readprev(struct log *log)
{

    if (log->position < LOG_ENTRYSIZE)
        return 0;

    log->position -= LOG_ENTRYSIZE;

    return log_read(log);

}

int log_find(struct log *log, char *id)
{

    unsigned int length = strlen(id);

    if (length > 32)
        return 0;

    if (length == 4 && memcmp(id, "HEAD", 4) == 0)
        return log_readprev(log);

    if (length > 5 && memcmp(id, "HEAD~", 5) == 0)
    {

        unsigned int count = 0;
        unsigned int i;
        int rc = log_readprev(log);

        sscanf(id, "HEAD~%u", &count);

        for (i = 0; i < count; i++)
        {

            rc = log_readprev(log);

            if (rc == 0)
                return 0;

        }

        return 1;

    }

    while (log_readprev(log))
    {

        if (memcmp(log->id, id, length) == 0)
            return 1;

    }

    return 0;

}

int log_printstd(struct log *log, unsigned int run, unsigned int descriptor)
{

    char buffer[BUFSIZ];
    unsigned int count;
    char path[BUFSIZ];
    int fd;

    switch (descriptor)
    {

    case 1:
        config_get_runpath(path, BUFSIZ, log->id, run, "stdout");

        break;

    case 2:
        config_get_runpath(path, BUFSIZ, log->id, run, "stderr");

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

int log_printrun(struct log *log, unsigned int run)
{

    char status[BUFSIZ];
    char remote[BUFSIZ];
    unsigned int count;
    char path[BUFSIZ];
    int fd;

    config_get_runpath(path, BUFSIZ, log->id, run, "remote");

    fd = open(path, O_RDONLY, 0644);

    if (fd < 0)
        return -1;

    count = read(fd, remote, BUFSIZ);
    remote[count - 1] = '\0';

    close(fd);

    config_get_runpath(path, BUFSIZ, log->id, run, "status");

    fd = open(path, O_RDONLY, 0644);

    if (fd < 0)
        return -1;

    count = read(fd, status, BUFSIZ);
    status[count - 1] = '\0';

    close(fd);

    printf("run=%u remote=%s status=%s\n", run, remote, status);

    return 0;

}

int log_print(struct log *log)
{

    char path[BUFSIZ];
    unsigned int i;

    config_get_rundirfull(path, BUFSIZ, log->id);
    printf("id=%s datetime=%s\n", log->id, log->datetime);
    printf("total=%u complete=%u aborted=%u passed=%u failed=%u\n", log->total, log->complete, log->aborted, log->passed, log->failed);
    printf("\n");

    for (i = 0; i < log->total; i++)
    {

        printf("    ");
        log_printrun(log, i);

    }

    printf("\n");

    return 0;

}

int log_add(struct log *log)
{

    char path[BUFSIZ];
    time_t timeraw;
    int fd;

    time(&timeraw);
    strftime(log->datetime, 25, "%FT%T%z", localtime(&timeraw));
    config_get_subpath(path, BUFSIZ, CONFIG_LOGS, "HEAD");

    fd = open(path, O_WRONLY | O_CREAT | O_APPEND | O_SYNC, 0644);

    if (fd < 0)
        return -1;

    flock(fd, LOCK_SH);

    if (dprintf(fd, "%s %s %04d %04d %04d %04d %04d\n", log->id, log->datetime, log->total, log->complete, log->aborted, log->passed, log->failed) != LOG_ENTRYSIZE)
        return -1;

    log->position = lseek(fd, -LOG_ENTRYSIZE, SEEK_CUR);

    flock(fd, LOCK_UN);
    close(fd);

    return 0;

}

int log_update(struct log *log)
{

    char path[BUFSIZ];
    int fd;

    config_get_subpath(path, BUFSIZ, CONFIG_LOGS, "HEAD");

    fd = open(path, O_WRONLY | O_SYNC, 0644);

    if (fd < 0)
        return -1;

    flock(fd, LOCK_SH);
    lseek(fd, log->position, SEEK_SET);

    if (dprintf(fd, "%s %s %04d %04d %04d %04d %04d\n", log->id, log->datetime, log->total, log->complete, log->aborted, log->passed, log->failed) != LOG_ENTRYSIZE)
        return -1;

    flock(fd, LOCK_UN);
    close(fd);

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

