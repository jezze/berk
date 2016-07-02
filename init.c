#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "berk.h"

void init_assert()
{

    if (access(BERK_CONFIG, F_OK) < 0)
        berk_panic("Current working directory is not an berk root folder.");

}

void init_setup()
{

    static const char *fmt =
        "[core]\n"
        "        version = %s\n";
    char path[1024];
    FILE *config;

    if (mkdir(BERK_ROOT, 0775) < 0)
        berk_panic("Ewok already initialized.");

    if (snprintf(path, 1024, "%s", BERK_CONFIG) < 0)
        berk_panic("Could not copy string.");

    config = fopen(path, "w");

    if (config == NULL)
        berk_panic("Could not create config file.");

    if (fprintf(config, fmt, BERK_VERSION) < 0)
        berk_panic("Could not write to config file.");

    fclose(config);

    if (snprintf(path, 1024, "%s", BERK_REMOTES_BASE) < 0)
        berk_panic("Could not copy string.");

    if (mkdir(path, 0775) < 0)
        berk_panic("Could not create directory.");

    if (snprintf(path, 1024, "%s", BERK_JOBS_BASE) < 0)
        berk_panic("Could not copy string.");

    if (mkdir(path, 0775) < 0)
        berk_panic("Could not create directory.");

    fprintf(stdout, "Initialized berk in '%s'\n", BERK_ROOT);

}

int init_parse(int argc, char **argv)
{

    if (argc)
        return berk_error_extra();

    init_setup();

    return EXIT_SUCCESS;

}

