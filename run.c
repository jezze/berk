#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "config.h"
#include "util.h"
#include "log.h"
#include "run.h"

int run_prepare(struct run *run, struct log_entry *entry)
{

    char path[BUFSIZ];

    config_get_rundir(path, BUFSIZ, entry->id, run->index);

    if (util_mkdir(path) < 0)
        return -1;

    return 0;

}

int run_update_remote(struct run *run, struct log_entry *entry, char *remote)
{

    char path[BUFSIZ];
    int fd;

    config_get_runpath(path, BUFSIZ, entry->id, run->index, "remote");

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd < 0)
        return -1;

    dprintf(fd, "%s\n", remote);
    close(fd);

    return 0;

}

int run_update_status(struct run *run, struct log_entry *entry, int status)
{

    char path[BUFSIZ];
    char *statusname;
    int fd;

    config_get_runpath(path, BUFSIZ, entry->id, run->index, "status");

    switch (status)
    {

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

int run_get_pid(struct run *run, struct log_entry *entry)
{

    char path[BUFSIZ];
    char buffer[64];
    unsigned int pid;
    int fd;

    config_get_runpath(path, BUFSIZ, entry->id, run->index, "pid");

    fd = open(path, O_RDONLY, 0644);

    if (fd < 0)
        return -1;

    read(fd, buffer, 64);

    sscanf(buffer, "%u\n", &pid);

    close(fd);

    return pid;

}

int run_update_pid(struct run *run, struct log_entry *entry, unsigned int pid)
{

    char path[BUFSIZ];
    int fd;

    config_get_runpath(path, BUFSIZ, entry->id, run->index, "pid");

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd < 0)
        return -1;

    dprintf(fd, "%u\n", pid);
    close(fd);

    return 0;

}

int run_open(struct run *run, struct log_entry *entry)
{

    char path[BUFSIZ];

    config_get_runpath(path, BUFSIZ, entry->id, run->index, "stderr");

    run->stderrfd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    config_get_runpath(path, BUFSIZ, entry->id, run->index, "stdout");

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

