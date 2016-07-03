#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "config.h"
#include "error.h"
#include "job.h"
#include "remote.h"
#include "con.h"
#include "con_ssh.h"

void run(struct job *job, struct remote *remote)
{

    int status;

    if (con_ssh_connect(remote) < 0)
        berk_panic("Could not connect to remote.");

    status = con_ssh_exec(remote, job->exec);

    if (con_ssh_disconnect(remote) < 0)
        berk_panic("Could not disconnect from remote.");

    fprintf(stdout, "status: %d\n", status);

}

