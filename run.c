#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "config.h"
#include "log.h"
#include "run.h"

int run_update_remote(struct run *run, struct log_entry *entry, char *remote)
{

    char path[BUFSIZ];
    int fd;

    if (config_get_rundir(path, BUFSIZ, entry->id, run->index))
        return -1;

    if (access(path, F_OK) && mkdir(path, 0775) < 0)
        return -1;

    if (config_get_runpath(path, BUFSIZ, entry->id, run->index, "remote"))
        return -1;

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

    if (config_get_rundir(path, BUFSIZ, entry->id, run->index))
        return -1;

    if (access(path, F_OK) && mkdir(path, 0775) < 0)
        return -1;

    if (config_get_runpath(path, BUFSIZ, entry->id, run->index, "status"))
        return -1;

    switch (status)
    {

    case RUN_STATUS_PENDING:
        statusname = "pending";

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

int run_open(struct run *run, struct log_entry *entry)
{

    char path[BUFSIZ];

    if (config_get_rundir(path, BUFSIZ, entry->id, run->index))
        return -1;

    if (access(path, F_OK) && mkdir(path, 0775) < 0)
        return -1;

    if (config_get_runpath(path, BUFSIZ, entry->id, run->index, "stderr"))
        return -1;

    run->stderrfd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (config_get_runpath(path, BUFSIZ, entry->id, run->index, "stdout"))
        return -1;

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

