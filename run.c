#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "config.h"
#include "util.h"
#include "run.h"

int run_prepare(struct run *run, char *id)
{

    char path[BUFSIZ];

    config_get_rundir(path, BUFSIZ, id, run->index);

    if (util_mkdir(path) < 0)
        return -1;

    return 0;

}

int run_update_remote(struct run *run, char *id, char *remote)
{

    char path[BUFSIZ];
    int fd;

    config_get_runpath(path, BUFSIZ, id, run->index, "remote");

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd < 0)
        return -1;

    dprintf(fd, "%s\n", remote);
    close(fd);

    return 0;

}

int run_get_status(struct run *run, char *id)
{

    char path[BUFSIZ];
    char buffer[64];
    unsigned int count;
    int fd;

    config_get_runpath(path, BUFSIZ, id, run->index, "status");

    fd = open(path, O_RDONLY, 0644);

    if (fd < 0)
        return -1;

    count = read(fd, buffer, 64);

    close(fd);

    if (count)
    {

        buffer[count - 1] = '\0';

        return util_hash(buffer);

    }

    return -1;

}

int run_update_status(struct run *run, char *id, unsigned int status)
{

    char path[BUFSIZ];
    char *statusname = "unknown";
    int fd;

    config_get_runpath(path, BUFSIZ, id, run->index, "status");

    switch (status)
    {

    case RUN_STATUS_UNKNOWN:
        statusname = "unknown";

        break;

    case RUN_STATUS_PENDING:
        statusname = "pending";

        break;

    case RUN_STATUS_ABORTED:
        statusname = "aborted";

        break;

    case RUN_STATUS_PASSED:
        statusname = "passed";

        break;

    case RUN_STATUS_FAILED:
        statusname = "failed";

        break;

    }

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd < 0)
        return -1;

    dprintf(fd, "%s\n", statusname);
    close(fd);

    return 0;

}

int run_get_pid(struct run *run, char *id)
{

    char path[BUFSIZ];
    char buffer[64];
    unsigned int count;
    int fd;

    config_get_runpath(path, BUFSIZ, id, run->index, "pid");

    fd = open(path, O_RDONLY, 0644);

    if (fd < 0)
        return -1;

    count = read(fd, buffer, 64);

    close(fd);

    if (count)
    {

        unsigned int pid;

        sscanf(buffer, "%u\n", &pid);

        return pid;

    }

    return -1;

}

int run_update_pid(struct run *run, char *id, unsigned int pid)
{

    char path[BUFSIZ];
    int fd;

    config_get_runpath(path, BUFSIZ, id, run->index, "pid");

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd < 0)
        return -1;

    dprintf(fd, "%u\n", pid);
    close(fd);

    return 0;

}

int run_open(struct run *run, char *id)
{

    char path[BUFSIZ];

    config_get_runpath(path, BUFSIZ, id, run->index, "stderr");

    run->stderrfd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    config_get_runpath(path, BUFSIZ, id, run->index, "stdout");

    run->stdoutfd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    return 0;

}

int run_close(struct run *run)
{

    close(run->stderrfd);
    close(run->stdoutfd);

    return 0;

}

void run_init(struct run *run, unsigned int index)
{

    memset(run, 0, sizeof (struct run));

    run->index = index;

}

