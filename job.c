#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include "berk.h"
#include "ini.h"
#include "init.h"
#include "job.h"
#include "remote.h"
#include "con_ssh.h"

void job_add(char *job_name)
{

    static const char *fmt =
        "[job]\n"
        "        name = %s\n"
        "        exec = whoami\n"
        "        label = all\n";
    char path[1024];
    FILE *config;

    init_assert();

    if (snprintf(path, 1024, "%s/%s", BERK_JOBS_BASE, job_name) < 0)
        berk_panic("Could not copy string.");

    config = fopen(path, "w");

    if (config == NULL)
        berk_panic("Could not create job config file.");

    if (fprintf(config, fmt, job_name) < 0)
        berk_panic("Could not write to job config file.");

    fclose(config);
    fprintf(stdout, "Job '%s' created in '%s'\n", job_name, path);

}

void job_copy(char *old_job_name, char *new_job_name)
{

}

void job_list()
{

    DIR *dir;
    struct dirent *entry;

    init_assert();

    dir = opendir(BERK_JOBS_BASE);

    if (dir == NULL)
        berk_panic("Could not open dir.");

    while ((entry = readdir(dir)) != NULL)
    {

        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        fprintf(stdout, "%s\n", entry->d_name);

    }

}

int job_load_callback(void *user, const char *section, const char *name, const char *value)
{

    struct job *job = user;

    if (!strcmp(section, "job") && !strcmp(name, "name"))
        strncpy(job->name, value, 32);

    if (!strcmp(section, "job") && !strcmp(name, "exec"))
        strncpy(job->exec, value, 1024);

    return 1;

}

int job_load(char *job_name, struct job *job)
{

    char path[1024];

    if (snprintf(path, 1024, "%s/%s", BERK_JOBS_BASE, job_name) < 0)
        return -1;

    memset(job, 0, sizeof (struct job));

    return ini_parse(path, job_load_callback, job);

}

void job_remove(char *job_name)
{

    char path[1024];

    init_assert();

    if (snprintf(path, 1024, "%s/%s", BERK_JOBS_BASE, job_name) < 0)
        berk_panic("Could not copy string.");

    if (unlink(path) < 0)
        berk_panic("Could not remove file.");

    fprintf(stdout, "Job '%s' removed from '%s'\n", job_name, path);

}

void job_start(char *job_name, char *remote_name)
{

    struct job job;
    struct remote remote;
    char exec[2048];
    unsigned int jobno = 583;
    int status;

    init_assert();

    if (job_load(job_name, &job) < 0)
        berk_panic("Could not load job.");

    if (remote_load(remote_name, &remote) < 0)
        berk_panic("Could not load remote.");

    if (snprintf(exec, 2048, "mkdir -p berk/%u && cd berk/%u && %s", jobno, jobno, job.exec) < 0)
        berk_panic("Could not copy string.");

    if (con_ssh_connect(&remote) < 0)
        berk_panic("Could not connect to remote.");

    status = con_ssh_exec(&remote, exec);

    if (con_ssh_disconnect(&remote) < 0)
        berk_panic("Could not disconnect from remote.");

    fprintf(stdout, "status: %d\n", status);

}

void job_stop(char *job_name)
{

    struct job job;

    init_assert();

    if (job_load(job_name, &job) < 0)
        berk_panic("Could not load job.");

}

static int job_parse_add(int argc, char **argv)
{

    if (argc < 1)
        return berk_error_missing();

    job_add(argv[0]);

    return EXIT_SUCCESS;

}

static int job_parse_copy(int argc, char **argv)
{

    if (argc < 2)
        return berk_error_missing();

    job_copy(argv[0], argv[1]);

    return EXIT_SUCCESS;

}

static int job_parse_list(int argc, char **argv)
{

    if (argc)
        return berk_error_extra();

    job_list();

    return EXIT_SUCCESS;

}

static int job_parse_remove(int argc, char **argv)
{

    if (argc < 1)
        return berk_error_missing();

    job_remove(argv[0]);

    return EXIT_SUCCESS;

}

static int job_parse_start(int argc, char **argv)
{

    if (argc < 2)
        return berk_error_missing();

    job_start(argv[0], argv[1]);

    return EXIT_SUCCESS;

}

static int job_parse_stop(int argc, char **argv)
{

    if (argc < 1)
        return berk_error_missing();

    job_stop(argv[0]);

    return EXIT_SUCCESS;

}

int job_parse(int argc, char **argv)
{

    static struct berk_command cmds[] = {
        {"add", job_parse_add, "<job-name>"},
        {"copy", job_parse_copy, "<old-job-name> <new-job-name>"},
        {"list", job_parse_list},
        {"remove", job_parse_remove, "<job-name>"},
        {"start", job_parse_start, "<job-name> <remote-name>"},
        {"stop", job_parse_stop, "<job-name>"},
        {0}
    };
    unsigned int i;

    if (argc < 1)
        return berk_error_command("job", cmds);

    for (i = 0; cmds[i].name; i++)
    {

        if (strcmp(argv[0], cmds[i].name))
            continue;

        return cmds[i].parse(argc - 1, argv + 1);

    }

    return berk_error_invalid(argv[0]);

}

