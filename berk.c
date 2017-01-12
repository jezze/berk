#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    int minargc;
    int maxargc;
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

        if ((argc - 1) < commands[i].minargc || (argc - 1) > commands[i].maxargc)
        {

            printf("Usage: %s %s%s\n", CONFIG_PROGNAME, commands[i].name, commands[i].usage);

            if (commands[i].description)            
                printf("\n%s", commands[i].description);

            return EXIT_SUCCESS;

        }

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

static int parseadd(int argc, char **argv)
{

    char *name = checkprint(argv[0]);
    char *hostname = checkprint(argv[1]);
    struct remote remote;

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

    char *name = checklist(argv[0]);
    char *key = (argc > 1) ? checkalpha(argv[1]) : NULL;
    char *value = (argc > 2) ? checkprint(argv[2]) : NULL;
    unsigned int names = util_split(name);
    struct remote remote;
    unsigned int i;

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

static int runexec(int gid, unsigned int pid, char *name, char *command)
{

    struct remote remote;
    int rc;

    if (remote_load(&remote, name))
        return errorload(name);

    if (remote_initoptional(&remote))
        return util_error("Could not init remote '%s'.", name);

    remote.pid = pid;

    if (remote_openlog(&remote, gid))
        return util_error("Could not open log.");

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

static int runsend(int gid, unsigned int pid, char *name, char *localpath, char *remotepath)
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

static int parseexec(int argc, char **argv)
{

    char *name = checklist(argv[0]);
    char *command = checkprint(argv[1]);
    unsigned int names = util_split(name);
    unsigned int total = 0;
    unsigned int complete = 0;
    unsigned int success = 0;
    unsigned int i;
    int gid = getpid();

    if (config_init())
        return errorinit();

    if (event_begin(gid))
        return util_error("Could not run event.");

    if (remote_logprepare(gid))
        return util_error("Could not prepare log.");

    for (i = 0; (name = util_nextword(name, i, names)); i++)
    {

        total++;

        if (runexec(gid, i, name, command) == 0)
            success++;

        complete++;

    }

    if (event_end(total, complete, success))
        return util_error("Could not run event.");

    if (remote_loghead(gid, total, complete, success))
        return util_error("Could not log HEAD.");

    return EXIT_SUCCESS;

}

static int parseexecp(int argc, char **argv)
{

    char *name = checklist(argv[0]);
    char *command = checkprint(argv[1]);
    unsigned int names = util_split(name);
    unsigned int total = 0;
    unsigned int complete = 0;
    unsigned int success = 0;
    unsigned int i;
    int gid = getpid();
    int status;

    if (config_init())
        return errorinit();

    if (event_begin(gid))
        return util_error("Could not run event.");

    if (remote_logprepare(gid))
        return util_error("Could not prepare log.");

    for (i = 0; (name = util_nextword(name, i, names)); i++)
    {

        pid_t pid = fork();

        if (pid == 0)
            return runexec(gid, i, name, command);

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

    if (event_end(total, complete, success))
        return util_error("Could not run event.");

    if (remote_loghead(gid, total, complete, success))
        return util_error("Could not log HEAD.");

    return EXIT_SUCCESS;

}

static int parseinit(int argc, char **argv)
{

    FILE *file;
    char path[BUFSIZ];
    char *hooks[] = {"begin", "end", "start", "stop", 0};
    unsigned int i;
    int fd;

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

    char *label = (argc > 0) ? checkprint(argv[0]) : NULL;
    DIR *dir;
    struct dirent *entry;
    char path[BUFSIZ];

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

static int parselog(int argc, char **argv)
{

    char *gid = (argc > 0) ? checkdigit(argv[0]) : NULL;
    char *pid = (argc > 1) ? checkdigit(argv[1]) : NULL;
    char path[BUFSIZ];

    if (config_init())
        return errorinit();

    if (gid)
    {

        if (pid)
        {

            int fd;
            char buffer[BUFSIZ];
            unsigned int count;

            if (config_getprocessbyname(path, BUFSIZ, gid, pid))
                return util_error("Could not get path.");

            fd = open(path, O_RDONLY, 0644);

            if (fd < 0)
                return util_error("Could not open '%s'.", path);

            while ((count = read(fd, buffer, BUFSIZ)))
                write(STDOUT_FILENO, buffer, count);

            close(fd);

        }

        else
        {

            DIR *dir;
            struct dirent *entry;

            if (config_getgroupbyname(path, BUFSIZ, gid))
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

        }

    }

    else
    {

        FILE *file;
        char buffer[BUFSIZ];

        if (config_getpath(path, BUFSIZ, CONFIG_LOGS "/HEAD"))
            return util_error("Could not get path.");

        file = fopen(path, "r");

        if (file == NULL)
            return EXIT_SUCCESS;

        while (fgets(buffer, BUFSIZ, file) != NULL)
            printf("%s", buffer);

        fclose(file);

    }

    return EXIT_SUCCESS;

}

static int parseremove(int argc, char **argv)
{

    char *name = checklist(argv[0]);
    unsigned int names = util_split(name);
    struct remote remote;
    unsigned int i;

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

    char *name = checklist(argv[0]);
    char *localpath = checkprint(argv[1]);
    char *remotepath = checkprint(argv[2]);
    unsigned int names = util_split(name);
    unsigned int i;
    int gid = getpid();

    if (config_init())
        return errorinit();

    for (i = 0; (name = util_nextword(name, i, names)); i++)
        runsend(gid, i, name, localpath, remotepath);

    return EXIT_SUCCESS;

}

static int parseshell(int argc, char **argv)
{

    char *name = checkprint(argv[0]);
    struct remote remote;

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
        {"add", parseadd, 2, 2, " <name> <hostname>", 0},
        {"config", parseconfig, 1, 3, " <namelist> [<key>] [<value>]", "List of keys:\n    name hostname port username privatekey publickey label\n"},
        {"exec", parseexec, 2, 2, " <namelist> <command>", 0},
        {"execp", parseexecp, 2, 2, " <namelist> <command>", 0},
        {"init", parseinit, 0, 0, "", 0},
        {"list", parselist, 0, 1, " [<label>]", 0},
        {"log", parselog, 0, 2, " [<gid>] [<pid>]", 0},
        {"remove", parseremove, 1, 1, " <namelist>", 0},
        {"send", parsesend, 3, 3, " <namelist> <localpath> <remotepath>", 0},
        {"shell", parseshell, 1, 1, " <name>", 0},
        {"version", parseversion, 0, 0, "", 0},
        {0}
    };

    return checkargs(commands, argc - 1, argv + 1);

}

