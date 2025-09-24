#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "config.h"
#include "util.h"
#include "run.h"

static char *getstatusname(unsigned int status)
{

    switch (status)
    {

    case RUN_STATUS_UNKNOWN:
        return "unknown";

    case RUN_STATUS_PENDING:
        return "pending";

    case RUN_STATUS_ABORTED:
        return "aborted";
        
    case RUN_STATUS_PASSED:
        return "passed";

    case RUN_STATUS_FAILED:
        return "failed";

    }

    return "unknown";

}

int run_load_pid(struct run *run)
{

    char path[BUFSIZ];
    int fd;

    run->pid = -1;

    config_get_runpath(path, BUFSIZ, run->id, run->index, "pid");

    fd = open(path, O_RDONLY, 0644);

    if (fd >= 0)
    {

        char buffer[64];
        unsigned int count = read(fd, buffer, 64);

        close(fd);

        if (count)
        {

            sscanf(buffer, "%d\n", &run->pid);

            return 0;

        }

    }

    return -1;

}

int run_load_status(struct run *run)
{

    char path[BUFSIZ];
    int fd;

    run->status = RUN_STATUS_UNKNOWN;

    config_get_runpath(path, BUFSIZ, run->id, run->index, "status");

    fd = open(path, O_RDONLY, 0644);

    if (fd >= 0)
    {

        char buffer[64];
        unsigned int count = read(fd, buffer, 64);

        close(fd);

        if (count)
        {

            buffer[count - 1] = '\0';

            run->status = util_hash(buffer);

            return 0;

        }

    }

    return -1;

}

int run_save_pid(struct run *run)
{

    char path[BUFSIZ];
    int fd;

    config_get_runpath(path, BUFSIZ, run->id, run->index, "pid");

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);

    if (fd >= 0)
    {

        dprintf(fd, "%u\n", run->pid);
        close(fd);

        return 0;

    }

    return -1;

}

int run_save_remote(struct run *run, char *remote)
{

    char path[BUFSIZ];
    int fd;

    config_get_runpath(path, BUFSIZ, run->id, run->index, "remote");

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);

    if (fd >= 0)
    {

        dprintf(fd, "%s\n", remote);
        close(fd);

        return 0;

    }

    return -1;

}

int run_save_status(struct run *run)
{

    char path[BUFSIZ];
    int fd;

    config_get_runpath(path, BUFSIZ, run->id, run->index, "status");

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);

    if (fd >= 0)
    {

        dprintf(fd, "%s\n", getstatusname(run->status));
        close(fd);

        return 0;

    }

    return -1;

}

int run_prepare(struct run *run)
{

    char path[BUFSIZ];

    config_get_rundir(path, BUFSIZ, run->id, run->index);

    if (util_mkdir(path) < 0)
        return -1;

    return 0;

}

void run_print(struct run *run)
{

    char remote[BUFSIZ];
    unsigned int count;
    char path[BUFSIZ];
    int fd;

    config_get_runpath(path, BUFSIZ, run->id, run->index, "remote");

    fd = open(path, O_RDONLY, 0644);

    if (fd >= 0)
    {

        count = read(fd, remote, BUFSIZ);
        remote[count - 1] = '\0';

    }

    else
    {

        strcpy(remote, "unknown");
        count = 7;

    }

    close(fd);

    printf("run=%u remote=%s status=%s\n", run->index, remote, getstatusname(run->status));

}

void run_printstd(struct run *run, unsigned int descriptor)
{

    char buffer[BUFSIZ];
    unsigned int count;
    char path[BUFSIZ];
    int fd;

    switch (descriptor)
    {

    case 1:
        config_get_runpath(path, BUFSIZ, run->id, run->index, "stdout");

        break;

    case 2:
        config_get_runpath(path, BUFSIZ, run->id, run->index, "stderr");

        break;

    }

    fd = open(path, O_RDONLY, 0644);

    if (fd >= 0)
    {

        while ((count = read(fd, buffer, BUFSIZ)))
            write(STDOUT_FILENO, buffer, count);

    }

    close(fd);

}

int run_openstd(struct run *run)
{

    char path[BUFSIZ];

    config_get_runpath(path, BUFSIZ, run->id, run->index, "stderr");

    run->stderrfd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);

    config_get_runpath(path, BUFSIZ, run->id, run->index, "stdout");

    run->stdoutfd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);

    return 0;

}

int run_closestd(struct run *run)
{

    close(run->stderrfd);
    close(run->stdoutfd);

    return 0;

}

void run_init(struct run *run, char *id, unsigned int index)
{

    memset(run, 0, sizeof (struct run));

    run->id = id;
    run->index = index;

}

