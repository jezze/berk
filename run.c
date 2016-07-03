#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "init.h"
#include "berk.h"
#include "job.h"
#include "remote.h"
#include "con.h"
#include "con_ssh.h"

void run(char *job_name, char *remote_name)
{

    struct job job;
    struct remote remote;
    int status;

    init_assert();

    if (job_load(job_name, &job) < 0)
        berk_panic("Could not load job.");

    if (remote_load(remote_name, &remote) < 0)
        berk_panic("Could not load remote.");

    if (con_ssh_connect(&remote) < 0)
        berk_panic("Could not connect to remote.");

    status = con_ssh_exec(&remote, job.exec);

    if (con_ssh_disconnect(&remote) < 0)
        berk_panic("Could not disconnect from remote.");

    fprintf(stdout, "status: %d\n", status);

}

int run_parse(int argc, char **argv)
{

    if (argc < 2)
        return berk_error_missing();

    run(argv[0], argv[1]);

    return EXIT_SUCCESS;

}

