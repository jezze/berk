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

static int job_load_callback(void *user, const char *section, const char *name, const char *value)
{

    struct job *job = user;

    if (!strcmp(section, "job") && !strcmp(name, "name"))
        job->name = strdup(value);

    if (!strcmp(section, "job") && !strcmp(name, "exec"))
        job->exec = strdup(value);

    return 1;

}

int job_load(char *filename, struct job *job)
{

    char path[1024];

    if (snprintf(path, 1024, "%s/%s", BERK_JOBS_BASE, filename) < 0)
        return -1;

    memset(job, 0, sizeof (struct job));

    return ini_parse(path, job_load_callback, job);

}

int job_save(char *filename, struct job *job)
{

    FILE *file = fopen(filename, "w");

    if (file == NULL)
        berk_panic("Could not create job config file.");

    ini_writesection(file, "job");
    ini_writestring(file, "name", job->name);
    ini_writestring(file, "exec", job->exec);
    fclose(file);

    return 0;

}

void job_copy(char *old_job_name, char *new_job_name)
{

}

void job_create(char *job_name)
{

    struct job job;
    char path[1024];

    init_assert();
    memset(&job, 0, sizeof (struct job));
    snprintf(path, 1024, "%s/%s", BERK_JOBS_BASE, job_name);

    job.name = job_name;
    job.exec = "whoami";

    job_save(path, &job);
    fprintf(stdout, "Job '%s' created in '%s'\n", job_name, path);

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

void job_show(char *job_name)
{

    struct job job;

    init_assert();

    if (job_load(job_name, &job) < 0)
        berk_panic("Could not load job.");

    fprintf(stdout, "name: %s\n", job.name);
    fprintf(stdout, "exec: %s\n", job.exec);

}

static int job_parse_copy(int argc, char **argv)
{

    if (argc < 2)
        return berk_error_missing();

    job_copy(argv[0], argv[1]);

    return EXIT_SUCCESS;

}

static int job_parse_create(int argc, char **argv)
{

    if (argc < 1)
        return berk_error_missing();

    job_create(argv[0]);

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

static int job_parse_show(int argc, char **argv)
{

    if (argc < 1)
        return berk_error_missing();

    job_show(argv[0]);

    return EXIT_SUCCESS;

}

int job_parse(int argc, char **argv)
{

    static struct berk_command cmds[] = {
        {"list", job_parse_list},
        {"show", job_parse_show, "<id>"},
        {"create", job_parse_create, "<id>"},
        {"remove", job_parse_remove, "<id>"},
        {"copy", job_parse_copy, "<id> <new-id>"},
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

