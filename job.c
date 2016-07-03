#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include "config.h"
#include "error.h"
#include "ini.h"
#include "job.h"

static int getconfigpath(char *path, unsigned int length, char *filename)
{

    return snprintf(path, 1024, "%s/%s", BERK_JOBS_BASE, filename) < 0;

}

static int loadcallback(void *user, const char *section, const char *name, const char *value)
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

    if (getconfigpath(path, 1024, filename))
        return -1;

    memset(job, 0, sizeof (struct job));

    return ini_parse(path, loadcallback, job);

}

int job_save(char *filename, struct job *job)
{

    FILE *file = fopen(filename, "w");

    if (file == NULL)
        return -1;

    ini_writesection(file, "job");
    ini_writestring(file, "name", job->name);
    ini_writestring(file, "exec", job->exec);
    fclose(file);

    return 0;

}

void job_copy(struct job *job, char *name)
{

    char path[1024];
    char *temp = job->name;

    job->name = name;

    getconfigpath(path, 1024, job->name);

    if (job_save(path, job))
        error(ERROR_PANIC, "Could not save '%s'.", path);

    job->name = temp;

    fprintf(stdout, "Job '%s' (copied from '%s') created in '%s'\n", name, job->name, path);

}

void job_create(char *name)
{

    struct job job;
    char path[1024];

    memset(&job, 0, sizeof (struct job));

    job.name = name;
    job.exec = "whoami";

    getconfigpath(path, 1024, job.name);

    if (job_save(path, &job))
        error(ERROR_PANIC, "Could not save '%s'.", path);

    fprintf(stdout, "Job '%s' created in '%s'\n", job.name, path);

}

void job_list()
{

    DIR *dir;
    struct dirent *entry;

    dir = opendir(BERK_JOBS_BASE);

    if (dir == NULL)
        error(ERROR_PANIC, "Could not open '%s'.", BERK_JOBS_BASE);

    while ((entry = readdir(dir)) != NULL)
    {

        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        fprintf(stdout, "%s\n", entry->d_name);

    }

}

void job_remove(struct job *job)
{

    char path[1024];

    getconfigpath(path, 1024, job->name);

    if (unlink(path) < 0)
        error(ERROR_PANIC, "Could not remove '%s'.", path);

    fprintf(stdout, "Job '%s' removed from '%s'\n", job->name, path);

}

void job_show(struct job *job)
{

    fprintf(stdout, "name: %s\n", job->name);
    fprintf(stdout, "exec: %s\n", job->exec);

}

