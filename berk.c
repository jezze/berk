#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <libssh2.h>
#include "config.h"
#include "util.h"
#include "ini.h"
#include "remote.h"
#include "event.h"
#include "ssh.h"

struct command
{

    char *name;
    int (*parse)(int argc, char **argv);
    char *usage;
    char *description;

};

static int errorinit()
{

    return util_error("Could not find '%s' directory.", CONFIG_ROOT);

}

static int errorload(char *name)
{

    return util_error("Could not load remote '%s'.", name);

}

static int errorsave(char *name)
{

    return util_error("Could not save remote '%s'.", name);

}

static int checkargs(struct command *commands, int argc, char **argv)
{

    unsigned int i;

    if (!argc)
    {

        printf("Usage: %s <command> [<args>]\n\n", CONFIG_PROGNAME);
        printf("List of commands:\n");

        for (i = 0; commands[i].name; i++)
            printf("    %s%s\n", commands[i].name, commands[i].usage);

        return EXIT_SUCCESS;

    }

    for (i = 0; commands[i].name; i++)
    {

        if (strcmp(argv[0], commands[i].name))
            continue;

        return commands[i].parse(argc - 1, argv + 1);

    }

    return util_error("Invalid argument '%s'.", argv[0]);

}

static char *checkalpha(char *arg)
{

    util_trim(arg);

    if (util_checkalpha(arg))
        exit(util_error("Could not parse alpha value '%s'.", arg));

    return arg;

}

static char *checkdigit(char *arg)
{

    util_trim(arg);

    if (util_checkdigit(arg))
        exit(util_error("Could not parse digit value '%s'.", arg));

    return arg;

}

static char *checkxdigit(char *arg)
{

    util_trim(arg);

    if (util_checkxdigit(arg))
        exit(util_error("Could not parse hex value '%s'.", arg));

    return arg;

}

static char *checkprint(char *arg)
{

    util_trim(arg);

    if (util_checkprint(arg))
        exit(util_error("Could not parse printable value '%s'.", arg));

    return arg;

}

static char *checklist(char *arg)
{

    util_trim(arg);
    util_strip(arg);

    if (util_checkprintspace(arg))
        exit(util_error("Could not parse list:\n%s.", arg));

    return arg;

}

static int runexec(char *id, unsigned int pid, char *name, char *command)
{

    struct remote remote;
    int rc;

    if (remote_load(&remote, name))
        return errorload(name);

    if (remote_initoptional(&remote))
        return util_error("Could not init remote '%s'.", name);

    remote.pid = pid;

    if (remote_createlog(&remote, id))
        return util_error("Could not create log.");

    if (remote_openlogstderr(&remote, id))
        return util_error("Could not open stderr log.");

    if (remote_openlogstdout(&remote, id))
        return util_error("Could not open stdout log.");

    if (event_start(&remote))
        return util_error("Could not run event.");

    if (ssh_connect(&remote))
        return util_error("Could not connect to remote '%s'.", remote.name);

    rc = ssh_exec(&remote, command);

    if (ssh_disconnect(&remote))
        return util_error("Could not disconnect from remote '%s'.", remote.name);

    if (event_stop(&remote, rc))
        return util_error("Could not run event.");

    if (remote_closelog(&remote))
        return util_error("Could not close log.");

    return rc;

}

static int runsend(char *id, unsigned int pid, char *name, char *localpath, char *remotepath)
{

    struct remote remote;
    int rc;

    if (remote_load(&remote, name))
        return errorload(name);

    if (remote_initoptional(&remote))
        return util_error("Could not init remote '%s'.", name);

    remote.pid = pid;

    if (ssh_connect(&remote))
        return util_error("Could not connect to remote '%s'.", remote.name);

    rc = ssh_send(&remote, localpath, remotepath);

    if (ssh_disconnect(&remote))
        return util_error("Could not disconnect from remote '%s'.", remote.name);

    return rc;

}

static void createid(char *dest, unsigned int length)
{

    char charset[] = "0123456789abcdef";
    unsigned int i;

    srand(time(NULL));

    for (i = 0; i < length; i++)
    {

        unsigned int index = (double)rand() / RAND_MAX * 16;

        dest[i] = charset[index];

    }

    dest[length - 1] = '\0';

}

static int parseadd(int argc, char **argv)
{

    unsigned int position = 0;
    char *hostname = NULL;
    struct remote remote;
    char *name = NULL;
    unsigned int i;

    for (i = 0; i < argc; i++)
    {

        char *k = argv[i];

        switch (k[0])
        {

        case '-':
            switch (k[1])
            {

            case '\0':
                util_error("Incorrect flag argument.");

                break;

            default:
                util_error("Unknown flag '%s'.", k);

                break;

            }

            break;

        default:
            switch (position)
            {

            case 0:
                if (checkprint(k))
                    name = k;

                break;

            case 1:
                if (checkprint(k))
                    hostname = k;

                break;

            default:
                util_error("Too many arguments");

                break;

            }

            position++;

            break;

        }

    }

    if (config_init())
        return errorinit();

    if (remote_initrequired(&remote, name, hostname))
        return util_error("Could not init remote '%s'.", name);

    if (remote_save(&remote))
        return errorsave(name);

    printf("Remote '%s' added.\n", name);

    return EXIT_SUCCESS;

}

static int parseconfig(int argc, char **argv)
{

    unsigned int position = 0;
    unsigned int names = 0;
    struct remote remote;
    char *value = NULL;
    char *name = NULL;
    char *key = NULL;
    unsigned int i;

    for (i = 0; i < argc; i++)
    {

        char *k = argv[i];

        switch (k[0])
        {

        case '-':
            switch (k[1])
            {

            case '\0':
                util_error("Incorrect flag argument.");

                break;

            default:
                util_error("Unknown flag '%s'.", k);

                break;

            }

            break;

        default:
            switch (position)
            {

            case 0:
                if (checklist(k))
                {

                    name = k;
                    names = util_split(name);

                }

                break;

            case 1:
                if (checkalpha(k))
                    key = k;

                break;

            case 2:
                if (checkprint(k))
                    value = k;

                break;

            default:
                util_error("Too many arguments");

                break;

            }

            position++;

            break;

        }

    }

    if (config_init())
        return errorinit();

    for (i = 0; (name = util_nextword(name, i, names)); i++)
    {

        if (remote_load(&remote, name))
            return errorload(name);

        if (value)
        {

            int keytype = remote_gettype(key);

            if (keytype == -1)
                return util_error("Invalid key '%s'.", key);

            if (remote_setvalue(&remote, keytype, value) == NULL)
                return util_error("Could not run configure remote '%s'.", remote.name);

            if (remote_save(&remote))
                return errorsave(name);

        }

        else
        {

            if (key)
            {

                int keytype = remote_gettype(key);
                char *value;

                if (keytype == -1)
                    return util_error("Invalid key '%s'.", key);

                value = remote_getvalue(&remote, keytype);

                printf("%s: %s\n", remote.name, value);

            }

            else
            {

                printf("name=%s\n", remote.name);
                printf("hostname=%s\n", remote.hostname);

                if (remote.port)
                    printf("port=%s\n", remote.port);

                if (remote.username)                
                    printf("username=%s\n", remote.username);

                if (remote.password)                
                    printf("password=%s\n", remote.password);

                if (remote.privatekey)
                    printf("privatekey=%s\n", remote.privatekey);

                if (remote.publickey)
                    printf("publickey=%s\n", remote.publickey);

                if (remote.label)
                    printf("label=%s\n", remote.label);

            }

        }

    }

    return EXIT_SUCCESS;

}

static int parseexec(int argc, char **argv)
{

    unsigned int parallel = 0;
    unsigned int position = 0;
    unsigned int names = 0;
    unsigned int total = 0;
    unsigned int complete = 0;
    unsigned int success = 0;
    char *command = NULL;
    char *name = NULL;
    unsigned int i;
    char id[32];

    for (i = 0; i < argc; i++)
    {

        char *k = argv[i];

        switch (k[0])
        {

        case '-':
            switch (k[1])
            {

            case 'p':
                parallel = 1;

                break;

            case '\0':
                util_error("Incorrect flag argument.");

                break;

            default:
                util_error("Unknown flag '%s'.", k);

                break;

            }

            break;

        default:
            switch (position)
            {

            case 0:
                if (checklist(k))
                {

                    name = k;
                    names = util_split(name);

                }

                break;

            case 1:
                if (checkprint(k))
                    command = k;

                break;

            default:
                util_error("Too many arguments");

                break;

            }

            position++;

            break;

        }

    }

    createid(id, 32);

    if (config_init())
        return errorinit();

    if (event_begin(id))
        return util_error("Could not run event.");

    if (remote_logprepare(id))
        return util_error("Could not prepare log.");

    if (parallel)
    {

        int status;

        for (i = 0; (name = util_nextword(name, i, names)); i++)
        {

            pid_t pid = fork();

            if (pid == 0)
                return runexec(id, i, name, command);

        }

        while (wait(&status) > 0)
        {

            total++;

            if (WIFEXITED(status))
            {

                complete++;

                if (WEXITSTATUS(status) == 0)
                    success++;

            }

        }

    }

    else
    {

        for (i = 0; (name = util_nextword(name, i, names)); i++)
        {

            total++;

            if (runexec(id, i, name, command) == 0)
                success++;

            complete++;

        }

    }

    if (event_end(total, complete, success))
        return util_error("Could not run event.");

    if (remote_loghead(id, total, complete, success))
        return util_error("Could not log HEAD.");

    return EXIT_SUCCESS;

}

static int parseinit(int argc, char **argv)
{

    char *hooks[] = {"begin", "end", "start", "stop", 0};
    unsigned int position = 0;
    char path[BUFSIZ];
    unsigned int i;
    FILE *file;
    int fd;

    for (i = 0; i < argc; i++)
    {

        char *k = argv[i];

        switch (k[0])
        {

        case '-':
            switch (k[1])
            {

            case '\0':
                util_error("Incorrect flag argument.");

                break;

            default:
                util_error("Unknown flag '%s'.", k);

                break;

            }

            break;

        default:
            switch (position)
            {

            default:
                util_error("Too many arguments");

                break;

            }

            position++;

            break;

        }

    }

    if (mkdir(CONFIG_ROOT, 0775) < 0)
        return util_error("Already initialized.");

    if (config_init())
        return errorinit();

    if (config_getpath(path, BUFSIZ, "config"))
        return util_error("Could not get path.");

    file = fopen(path, "w");

    if (file == NULL)
        return util_error("Could not create config file.");

    ini_writesection(file, "core");
    ini_writestring(file, "version", CONFIG_VERSION);
    fclose(file);

    if (config_getpath(path, BUFSIZ, CONFIG_HOOKS))
        return util_error("Could not get path.");

    if (mkdir(path, 0775) < 0)
        return util_error("Could not create directory '%s'.", CONFIG_HOOKS);

    for (i = 0; hooks[i]; i++)
    {

        char buffer[BUFSIZ];
        unsigned int count;

        snprintf(buffer, BUFSIZ, "%s/%s.sample", CONFIG_HOOKS, hooks[i]);

        if (config_getpath(path, BUFSIZ, buffer))
            return util_error("Could not get path.");

        fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0755);

        if (fd < 0)
            return util_error("Could not create hook file '%s'.", hooks[i]);

        count = snprintf(buffer, BUFSIZ, "#!/bin/sh\n#\n# To enable this hook, rename this file to \"%s\".\n", hooks[i]);

        write(fd, buffer, count);
        close(fd);

    }

    printf("Initialized %s in '%s'.\n", CONFIG_PROGNAME, CONFIG_ROOT);

    return EXIT_SUCCESS;

}

static int parselist(int argc, char **argv)
{

    unsigned int position = 0;
    struct dirent *entry;
    char *label = NULL;
    char path[BUFSIZ];
    unsigned int i;
    DIR *dir;

    for (i = 0; i < argc; i++)
    {

        char *k = argv[i];

        switch (k[0])
        {

        case '-':
            switch (k[1])
            {

            case '\0':
                util_error("Incorrect flag argument.");

                break;

            default:
                util_error("Unknown flag '%s'.", k);

                break;

            }

            break;

        default:
            switch (position)
            {

            case 0:
                if (checkprint(k))
                    label = k;

                break;

            default:
                util_error("Too many arguments");

                break;

            }

            position++;

            break;

        }

    }

    if (config_init())
        return errorinit();

    if (config_getpath(path, BUFSIZ, CONFIG_REMOTES))
        return util_error("Could not get path.");

    dir = opendir(path);

    if (dir == NULL)
        return util_error("Could not open '%s'.", path);

    while ((entry = readdir(dir)) != NULL)
    {

        struct remote remote;
        unsigned int words;
        unsigned int i;

        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        if (remote_load(&remote, entry->d_name))
            continue;

        if (!label)
        {

            printf("%s\n", remote.name);

            continue;

        }

        if (!remote.label)
            continue;

        words = util_split(remote.label);

        for (i = 0; (remote.label = util_nextword(remote.label, i, words)); i++)
        {

            if (!strcmp(remote.label, label))
            {

                printf("%s\n", remote.name);

                break;

            }

        }

    }

    closedir(dir);

    return EXIT_SUCCESS;

}

static int parselog_readlog(char *id, char *pid, unsigned int descriptor)
{

    char buffer[BUFSIZ];
    unsigned int count;
    char path[BUFSIZ];
    int fd;

    switch (descriptor)
    {

    case 1:
        if (config_getlogsstdout(path, BUFSIZ, id, pid))
            return util_error("Could not get path.");

        break;

    case 2:
        if (config_getlogsstderr(path, BUFSIZ, id, pid))
            return util_error("Could not get path.");

        break;

    }

    fd = open(path, O_RDONLY, 0644);

    if (fd < 0)
        return util_error("Could not open '%s'.", path);

    while ((count = read(fd, buffer, BUFSIZ)))
        write(STDOUT_FILENO, buffer, count);

    close(fd);

    return EXIT_SUCCESS;

}

static int parselog_readrun(char *id)
{

    struct dirent *entry;
    char path[BUFSIZ];
    DIR *dir;

    if (config_getfullrun(path, BUFSIZ, id))
        return util_error("Could not get path.");

    dir = opendir(path);

    if (dir == NULL)
        return util_error("Could not open '%s'.", path);

    while ((entry = readdir(dir)) != NULL)
    {

        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        printf("%s\n", entry->d_name);

    }

    closedir(dir);

    return EXIT_SUCCESS;

}

static int parselog_readhead(void)
{

    char path[BUFSIZ];
    char buffer[73];
    FILE *file;

    if (config_getpath(path, BUFSIZ, CONFIG_LOGS "/HEAD"))
        return util_error("Could not get path.");

    file = fopen(path, "r");

    if (file == NULL)
        return EXIT_SUCCESS;

    buffer[72] = '\0';

    while (fgets(buffer, 72, file) != NULL)
    {

        char id[33];
        char datetime[25];
        unsigned int total;
        unsigned int complete;
        unsigned int success;
        int result = sscanf(buffer, "%s %s %u %u %u\n", id, datetime, &total, &complete, &success);

        if (result == 5)
        {

            printf("id             %s\n", id);
            printf("total          %04u\n", total);
            printf("complete       %04u/%04u (%04u)\n", complete, total, total - complete);
            printf("successful     %04u/%04u (%04u)\n", success, total, total - success);
            printf("datetime       %s\n\n", datetime);

        }

    }

    fclose(file);

    return EXIT_SUCCESS;

}

static int parselog(int argc, char **argv)
{

    unsigned int descriptor = 1;
    unsigned int position = 0;
    char *pid = NULL;
    char *id = NULL;
    unsigned int i;

    for (i = 0; i < argc; i++)
    {

        char *k = argv[i];

        switch (k[0])
        {

        case '-':
            switch (k[1])
            {

            case 'e':
                descriptor = 2;

                break;

            case '\0':
                util_error("Incorrect flag argument.");

                break;

            default:
                util_error("Unknown flag '%s'.", k);

                break;

            }

            break;

        default:
            switch (position)
            {

            case 0:
                if (checkxdigit(k))
                    id = k;

                break;

            case 1:
                if (checkdigit(k))
                    pid = k;

                break;

            default:
                util_error("Too many arguments");

                break;

            }

            position++;

            break;

        }

    }

    if (config_init())
        return errorinit();

    if (id && pid)
        return parselog_readlog(id, pid, descriptor);
    else if (id)
        return parselog_readrun(id);

    return parselog_readhead();

}

static int parseremove(int argc, char **argv)
{

    unsigned int position = 0;
    unsigned int names = 0;
    struct remote remote;
    char *name = NULL;
    unsigned int i;

    for (i = 0; i < argc; i++)
    {

        char *k = argv[i];

        switch (k[0])
        {

        case '-':
            switch (k[1])
            {

            case '\0':
                util_error("Incorrect flag argument.");

                break;

            default:
                util_error("Unknown flag '%s'.", k);

                break;

            }

            break;

        default:
            switch (position)
            {

            case 0:
                if (checklist(k))
                {

                    name = k;
                    names = util_split(name);

                }

                break;

            default:
                util_error("Too many arguments");

                break;

            }

            position++;

            break;

        }

    }

    if (config_init())
        return errorinit();

    for (i = 0; (name = util_nextword(name, i, names)); i++)
    {

        if (remote_load(&remote, name))
            return errorload(name);

        if (remote_erase(&remote))
            return util_error("Could not remove remote '%s'.", remote.name);

        printf("Remote '%s' removed.\n", remote.name);

    }

    return EXIT_SUCCESS;

}

static int parsesend(int argc, char **argv)
{

    unsigned int position = 0;
    char *remotepath = NULL;
    char *localpath = NULL;
    unsigned int names = 0;
    char *name = NULL;
    unsigned int i;
    char id[32];

    for (i = 0; i < argc; i++)
    {

        char *k = argv[i];

        switch (k[0])
        {

        case '-':
            switch (k[1])
            {

            case '\0':
                util_error("Incorrect flag argument.");

                break;

            default:
                util_error("Unknown flag '%s'.", k);

                break;

            }

            break;

        default:
            switch (position)
            {

            case 0:
                if (checklist(k))
                {

                    name = k;
                    names = util_split(k);

                }

                break;

            case 1:
                if (checkprint(k))
                    localpath = k;

                break;

            case 2:
                if (checkprint(k))
                    remotepath = k;

                break;

            default:
                util_error("Too many arguments");

                break;

            }

            position++;

            break;

        }

    }

    createid(id, 32);

    if (config_init())
        return errorinit();

    for (i = 0; (name = util_nextword(name, i, names)); i++)
        runsend(id, i, name, localpath, remotepath);

    return EXIT_SUCCESS;

}

static int parseshell(int argc, char **argv)
{

    struct remote remote;
    char *name = NULL;
    unsigned int position = 0;
    unsigned int i;

    for (i = 0; i < argc; i++)
    {

        char *k = argv[i];

        switch (k[0])
        {

        case '-':
            switch (k[1])
            {

            case '\0':
                util_error("Incorrect flag argument.");

                break;

            default:
                util_error("Unknown flag '%s'.", k);

                break;

            }

            break;

        default:
            switch (position)
            {

            case 0:
                if (checkprint(k))
                    name = k;

                break;

            default:
                util_error("Too many arguments");

                break;

            }

            position++;

            break;

        }

    }

    if (config_init())
        return errorinit();

    if (remote_load(&remote, name))
        return errorload(name);

    if (remote_initoptional(&remote))
        return util_error("Could not init remote '%s'.", name);

    if (ssh_connect(&remote))
        return util_error("Could not connect to remote '%s'.", remote.name);

    if (ssh_shell(&remote))
        return util_error("Could not open shell on remote '%s'.", remote.name);

    if (ssh_disconnect(&remote))
        return util_error("Could not disconnect from remote '%s'.", remote.name);

    return EXIT_SUCCESS;

}

static int parseversion(int argc, char **argv)
{

    printf("%s version %s\n", CONFIG_PROGNAME, CONFIG_VERSION);

    return EXIT_SUCCESS;

}

int main(int argc, char **argv)
{

    static struct command commands[] = {
        {"add", parseadd, " <name> <hostname>", 0},
        {"config", parseconfig, " <namelist> [<key>] [<value>]", "List of keys:\n    name hostname port username password privatekey publickey label\n"},
        {"exec", parseexec, " [-p] <namelist> <command>", "Args:\n    -p  Run in parallel\n"},
        {"init", parseinit, "", 0},
        {"list", parselist, " [<label>]", 0},
        {"log", parselog, " [-e] [<id>] [<pid>]", "Args:\n    -e  Show stderr\n"},
        {"remove", parseremove, " <namelist>", 0},
        {"send", parsesend, " <namelist> <localpath> <remotepath>", 0},
        {"shell", parseshell, " <name>", 0},
        {"version", parseversion, "", 0},
        {0}
    };

    return checkargs(commands, argc - 1, argv + 1);

}

