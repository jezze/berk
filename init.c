#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "config.h"
#include "error.h"
#include "ini.h"

static int saveconfig(char *filename, char *version)
{

    FILE *file = fopen(filename, "w");

    if (file == NULL)
        error(ERROR_PANIC, "Could not create config file.");

    ini_writesection(file, "core");
    ini_writestring(file, "version", version);
    fclose(file);

    return 0;

}

void init_assert()
{

    if (access(BERK_CONFIG, F_OK) < 0)
        error(ERROR_PANIC, "Could not find '%s' directory.", BERK_ROOT);

}

void init_setup()
{

    char path[1024];

    if (mkdir(BERK_ROOT, 0775) < 0)
        error(ERROR_PANIC, "Already initialized.");

    if (snprintf(path, 1024, "%s", BERK_CONFIG) < 0)
        error(ERROR_PANIC, "Could not copy string.");

    saveconfig(path, BERK_VERSION);

    if (snprintf(path, 1024, "%s", BERK_REMOTES_BASE) < 0)
        error(ERROR_PANIC, "Could not copy string.");

    if (mkdir(path, 0775) < 0)
        error(ERROR_PANIC, "Could not create directory.");

    if (snprintf(path, 1024, "%s", BERK_JOBS_BASE) < 0)
        error(ERROR_PANIC, "Could not copy string.");

    if (mkdir(path, 0775) < 0)
        error(ERROR_PANIC, "Could not create directory.");

    fprintf(stdout, "Initialized %s in '%s'\n", BERK_NAME, BERK_ROOT);

}

