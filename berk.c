#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>
#include <libssh2.h>
#include "config.h"
#include "util.h"
#include "ini.h"
#include "log.h"
#include "remote.h"
#include "run.h"
#include "event.h"
#include "ssh.h"

struct command
{

    char *name;
    int (*parse)(int argc, char **argv);
    char *usage;
    char *description;
    unsigned int needconfig;

};

static int error(char *format, ...)
{

    va_list args;

    va_start(args, format);
    fprintf(stderr, "%s: ", CONFIG_PROGNAME);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);

    return EXIT_FAILURE;

}

static int error_init(void)
{

    return error("Could not find '%s' directory.", CONFIG_ROOT);

}

static int error_remote_init(char *name)
{

    return error("Could not init remote '%s'.", name);

}

static int error_remote_load(char *name)
{

    return error("Could not load remote '%s'.", name);

}

static int error_remote_save(char *name)
{

    return error("Could not save remote '%s'.", name);

}

static int error_missing(void)
{

    return error("Missing arguments.");

}

static int error_toomany(void)
{

    return error("Too many arguments.");

}

static int error_flag_unrecognized(char *arg)
{

    return error("Unrecognized flag '%s'.", arg);

}

static int assert_args(struct command *commands, int argc, char **argv)
{

    unsigned int i;

    if (!argc)
    {

        printf("Usage: %s <command> [<args>]\n\n", CONFIG_PROGNAME);
        printf("List of commands:\n");

        for (i = 0; commands[i].name; i++)
            printf("    %s\n", commands[i].usage);

        return EXIT_SUCCESS;

    }

    for (i = 0; commands[i].name; i++)
    {

        if (strcmp(argv[0], commands[i].name))
            continue;

        if (commands[i].needconfig)
        {

            if (!config_init())
                return error_init();

        }

        return commands[i].parse(argc - 1, argv + 1);

    }

    return error("Invalid argument '%s'.", argv[0]);

}

static char *assert_alpha(char *arg)
{

    util_trim(arg);

    if (!util_assert_alpha(arg))
        exit(error("Could not parse alpha value '%s'.", arg));

    return arg;

}

static char *assert_digit(char *arg)
{

    util_trim(arg);

    if (!util_assert_digit(arg))
        exit(error("Could not parse digit value '%s'.", arg));

    return arg;

}

static char *assert_print(char *arg)
{

    util_trim(arg);

    if (!util_assert_print(arg))
        exit(error("Could not parse printable value '%s'.", arg));

    return arg;

}

static char *assert_list(char *arg)
{

    util_trim(arg);
    util_strip(arg);

    if (!util_assert_printspace(arg))
        exit(error("Could not parse list:\n%s.", arg));

    return arg;

}

static int run_exec(struct log_entry *entry, unsigned int pid, unsigned int index, char *name, char *command)
{

    struct remote remote;
    struct run run;
    int rc;

    remote_init(&remote, name);
    run_init(&run, index);

    if (remote_load(&remote))
        return error_remote_load(name);

    if (remote_init_optional(&remote))
        return error_remote_init(name);

    if (run_prepare(&run, entry))
        return error("Could not prepare run.");

    if (run_update_remote(&run, entry, name))
        return error("Could not update run remote.");

    if (run_update_pid(&run, entry, pid))
        return error("Could not update run pid.");

    if (run_update_status(&run, entry, RUN_STATUS_PENDING))
        return error("Could not update run status.");

    if (run_open(&run, entry))
        return error("Could not open run.");

    if (event_start(&remote, &run))
        return error("Could not run event.");

    if (ssh_connect(&remote))
        return error("Could not connect to remote '%s'.", remote.name);

    rc = ssh_exec(&remote, &run, command);

    if (run_update_pid(&run, entry, 0))
        return error("Could not update run pid.");

    if (rc == 0)
    {

        if (run_update_status(&run, entry, RUN_STATUS_PASSED))
            return error("Could not update run status.");

        entry->passed++;

    }

    else
    {

        if (run_update_status(&run, entry, RUN_STATUS_FAILED))
            return error("Could not update run status.");

        entry->failed++;

    }

    if (ssh_disconnect(&remote))
        return error("Could not disconnect from remote '%s'.", remote.name);

    if (event_stop(&remote, &run, rc))
        return error("Could not run event.");

    if (run_close(&run))
        return error("Could not close run.");

    entry->complete++;

    return rc;

}

static int run_send(char *name, char *localpath, char *remotepath)
{

    struct remote remote;
    int rc;

    remote_init(&remote, name);

    if (remote_load(&remote))
        return error_remote_load(name);

    if (remote_init_optional(&remote))
        return error_remote_init(name);

    if (ssh_connect(&remote))
        return error("Could not connect to remote '%s'.", remote.name);

    rc = ssh_send(&remote, localpath, remotepath);

    if (ssh_disconnect(&remote))
        return error("Could not disconnect from remote '%s'.", remote.name);

    return rc;

}

static int parse_add(int argc, char **argv)
{

    char *hostname = NULL;
    char *name = NULL;
    unsigned int argp = 0;
    unsigned int argi;

    for (argi = 0; argi < argc; argi++)
    {

        char *arg = argv[argi];

        if (arg[0] == '-')
        {

            return error_flag_unrecognized(arg);

        }

        else
        {

            switch (argp++)
            {

            case 0:
                name = assert_print(arg);

                break;

            case 1:
                hostname = assert_print(arg);

                break;

            default:
                return error_toomany();

            }

        }

    }

    if (name && hostname)
    {

        struct remote remote;

        remote_init(&remote, name);
        remote_set_value(&remote, REMOTE_HOSTNAME, hostname);

        if (remote_save(&remote))
            return error_remote_save(name);

        printf("Remote '%s' added.\n", name);

        return EXIT_SUCCESS;

    }

    return error_missing();

}

static int parse_config(int argc, char **argv)
{

    char *value = NULL;
    char *name = NULL;
    char *key = NULL;
    unsigned int argp = 0;
    unsigned int argi;

    for (argi = 0; argi < argc; argi++)
    {

        char *arg = argv[argi];

        if (arg[0] == '-')
        {

            return error_flag_unrecognized(arg);

        }

        else
        {

            switch (argp++)
            {

            case 0:
                name = assert_list(arg);

                break;

            case 1:
                key = assert_alpha(arg);

                break;

            case 2:
                value = assert_print(arg);

                break;

            default:
                return error_toomany();

            }

        }

    }

    if (name && key && value)
    {

        unsigned int names = util_split(name);
        unsigned int keyhash = util_hash(key);
        unsigned int i;

        for (i = 0; (name = util_nextword(name, i, names)); i++)
        {

            struct remote remote;

            remote_init(&remote, name);

            if (remote_load(&remote))
                return error_remote_load(name);

            if (remote_set_value(&remote, keyhash, value) == NULL)
                return error("Could not run configure remote '%s'.", remote.name);

            if (remote_save(&remote))
                return error_remote_save(name);

        }

        return EXIT_SUCCESS;

    }

    else if (name && key)
    {

        unsigned int names = util_split(name);
        unsigned int keyhash = util_hash(key);
        unsigned int i;

        for (i = 0; (name = util_nextword(name, i, names)); i++)
        {

            struct remote remote;
            char *value;

            remote_init(&remote, name);

            if (remote_load(&remote))
                return error_remote_load(name);

            value = remote_get_value(&remote, keyhash);

            printf("%s: %s\n", remote.name, value);

        }

        return EXIT_SUCCESS;

    }

    else if (name)
    {

        unsigned int names = util_split(name);
        unsigned int i;

        for (i = 0; (name = util_nextword(name, i, names)); i++)
        {

            struct remote remote;

            remote_init(&remote, name);

            if (remote_load(&remote))
                return error_remote_load(name);

            if (remote.name)
                printf("name=%s\n", remote.name);

            if (remote.hostname)
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

            if (remote.tags)
                printf("tags=%s\n", remote.tags);

        }

        return EXIT_SUCCESS;

    }

    return error_missing();

}

static int parse_exec(int argc, char **argv)
{

    unsigned int doseq = 0;
    unsigned int dowait = 0;
    unsigned int nofork = 0;
    char *command = NULL;
    char *name = NULL;
    unsigned int argp = 0;
    unsigned int argi;

    for (argi = 0; argi < argc; argi++)
    {

        char *arg = argv[argi];

        if (arg[0] == '-')
        {

            switch (arg[1])
            {

            case 'n':
                nofork = 1;

                break;

            case 's':
                doseq = 1;

                break;

            case 'w':
                dowait = 1;

                break;

            default:
                return error_flag_unrecognized(arg);

            }

        }

        else
        {

            switch (argp++)
            {

            case 0:
                name = assert_list(arg);

                break;

            case 1:
                command = arg;

                break;

            default:
                return error_toomany();

            }

        }

    }

    if (name && command)
    {

        struct log_entry entry;
        unsigned int names = util_split(name);

        log_entry_init(&entry);

        if (event_begin(&entry))
            return error("Could not run event.");

        if (log_entry_prepare(&entry))
            return error("Could not prepare log.");

        entry.total = names;

        if (nofork)
        {

            unsigned int i;

            for (i = 0; (name = util_nextword(name, i, names)); i++)
                run_exec(&entry, 0, i, name, command);

        }

        else
        {

            unsigned int i;
            int status = 0;

            for (i = 0; (name = util_nextword(name, i, names)); i++)
            {

                pid_t pid = fork();

                if (pid == 0)
                    return run_exec(&entry, getpid(), i, name, command);

                if (doseq)
                    waitpid(pid, &status, 0);

            }

            if (dowait)
                while (wait(&status) > 0);

        }

        if (event_end(&entry))
            return error("Could not run event.");

        if (log_entry_write(&entry))
            return error("Could not log HEAD.");

        return EXIT_SUCCESS;

    }

    return error_missing();

}

static int parse_init(int argc, char **argv)
{

    unsigned int argi;

    for (argi = 0; argi < argc; argi++)
    {

        char *arg = argv[argi];

        if (arg[0] == '-')
            return error_flag_unrecognized(arg);
        else
            return error_toomany();

    }

    {

        char *hooks[] = {"begin", "end", "start", "stop", 0};
        unsigned int i;
        char path[BUFSIZ];
        FILE *file;
        int fd;

        if (mkdir(CONFIG_ROOT, 0775) < 0)
            return error("Already initialized.");

        if (!config_init())
            return error_init();

        config_get_path(path, BUFSIZ, "config");

        file = fopen(path, "w");

        if (file == NULL)
            return error("Could not create config file.");

        ini_write_section(file, "core");
        ini_write_string(file, "version", CONFIG_VERSION);
        fclose(file);

        config_get_path(path, BUFSIZ, CONFIG_HOOKS);

        if (mkdir(path, 0775) < 0)
            return error("Could not create directory '%s'.", CONFIG_HOOKS);

        for (i = 0; hooks[i]; i++)
        {

            char buffer[BUFSIZ];

            snprintf(buffer, BUFSIZ, "%s.sample", hooks[i]);
            config_get_subpath(path, BUFSIZ, CONFIG_HOOKS, buffer);

            fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0755);

            if (fd < 0)
                return error("Could not create hook file '%s'.", hooks[i]);

            dprintf(fd, "#!/bin/sh\n#\n# To enable this hook, rename this file to \"%s\".\n", hooks[i]);
            close(fd);

        }

        printf("Initialized %s in '%s'.\n", CONFIG_PROGNAME, CONFIG_ROOT);

        return EXIT_SUCCESS;

    }

    return error_missing();

}

static int parse_list(int argc, char **argv)
{

    char *tags = NULL;
    unsigned int argi;

    for (argi = 0; argi < argc; argi++)
    {

        char *arg = argv[argi];

        if (arg[0] == '-')
        {

            switch (arg[1])
            {

            case 't':
                tags = assert_print(argv[++argi]);

                break;

            default:
                return error_flag_unrecognized(arg);

            }

        }

        else
        {

            return error_toomany();

        }

    }

    {

        struct dirent *entry;
        char path[BUFSIZ];
        DIR *dir;

        config_get_path(path, BUFSIZ, CONFIG_REMOTES);

        dir = opendir(path);

        if (dir == NULL)
            return error("Could not open '%s'.", path);

        while ((entry = readdir(dir)) != NULL)
        {

            struct remote remote;
            unsigned int words;
            unsigned int i;

            if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
                continue;

            remote_init(&remote, entry->d_name);

            if (remote_load(&remote))
                continue;

            if (!tags)
            {

                printf("%s\n", remote.name);

                continue;

            }

            if (!remote.tags)
                continue;

            words = util_split(remote.tags);

            for (i = 0; (remote.tags = util_nextword(remote.tags, i, words)); i++)
            {

                if (!strcmp(remote.tags, tags))
                {

                    printf("%s\n", remote.name);

                    break;

                }

            }

        }

        closedir(dir);

        return EXIT_SUCCESS;

    }

    return error_missing();

}

static int parse_log(int argc, char **argv)
{

    unsigned int descriptor = 1;
    char *count = "0";
    char *skip = "0";
    char *run = NULL;
    char *id = NULL;
    unsigned int argp = 0;
    unsigned int argi;

    for (argi = 0; argi < argc; argi++)
    {

        char *arg = argv[argi];

        if (arg[0] == '-')
        {

            switch (arg[1])
            {

            case 'c':
                count = assert_digit(argv[++argi]);

                break;

            case 'e':
                descriptor = 2;

                break;

            case 's':
                skip = assert_digit(argv[++argi]);

                break;

            default:
                return error_flag_unrecognized(arg);

            }

        }

        else
        {

            switch (argp++)
            {

            case 0:
                id = assert_print(arg);

                break;

            case 1:
                run = assert_digit(arg);

                break;

            default:
                return error_toomany();

            }

        }

    }

    if (id && run)
    {

        struct log_entry entry;
        struct log_state state;
        unsigned int r = strtoul(run, NULL, 10);

        if (log_state_open(&state) < 0)
            return error("Unable to open state.");

        if (log_entry_find(&entry, &state, id))
            log_entry_printstd(&entry, r, descriptor);

        if (log_state_close(&state) < 0)
            return error("Unable to close state.");

        return EXIT_SUCCESS;

    }

    else if (id)
    {

        struct log_entry entry;
        struct log_state state;

        if (log_state_open(&state) < 0)
            return error("Unable to open state.");

        if (log_entry_find(&entry, &state, id))
            log_entry_print(&entry);

        if (log_state_close(&state) < 0)
            return error("Unable to close state.");

        return EXIT_SUCCESS;

    }

    else
    {

        struct log_entry entry;
        struct log_state state;
        unsigned int s = strtoul(skip, NULL, 10);
        unsigned int c = strtoul(count, NULL, 10);
        unsigned int n;

        if (log_state_open(&state) < 0)
            return error("Unable to open state.");

        for (n = 1; log_entry_readprev(&entry, &state); n++)
        {

            if (n > s)
                log_entry_print(&entry);

            if (c && n - s == c)
                break;

        }

        if (log_state_close(&state) < 0)
            return error("Unable to close state.");

        return EXIT_SUCCESS;

    }

    return error_missing();

}

static int parse_remove(int argc, char **argv)
{

    char *name = NULL;
    unsigned int argp = 0;
    unsigned int argi;

    for (argi = 0; argi < argc; argi++)
    {

        char *arg = argv[argi];

        if (arg[0] == '-')
        {

            return error_flag_unrecognized(arg);

        }

        else
        {

            switch (argp++)
            {

            case 0:
                name = assert_list(arg);

                break;

            default:
                return error_toomany();

            }

        }

    }

    if (name)
    {

        unsigned int names = util_split(name);
        unsigned int i;

        for (i = 0; (name = util_nextword(name, i, names)); i++)
        {

            struct remote remote;

            remote_init(&remote, name);

            if (remote_load(&remote))
                return error_remote_load(name);

            if (remote_erase(&remote))
                return error("Could not remove remote '%s'.", remote.name);

            printf("Remote '%s' removed.\n", remote.name);

        }

        return EXIT_SUCCESS;

    }

    return error_missing();

}

static int parse_send(int argc, char **argv)
{

    char *name = NULL;
    char *remotepath = NULL;
    char *localpath = NULL;
    unsigned int argp = 0;
    unsigned int argi;

    for (argi = 0; argi < argc; argi++)
    {

        char *arg = argv[argi];

        if (arg[0] == '-')
        {

            return error_flag_unrecognized(arg);

        }

        else
        {

            switch (argp++)
            {

            case 0:
                name = assert_list(arg);

                break;

            case 1:
                localpath = assert_print(arg);

                break;

            case 2:
                remotepath = assert_print(arg);

                break;

            default:
                return error_toomany();

            }

        }

    }

    if (name && localpath && remotepath)
    {

        unsigned int names = util_split(name);
        unsigned int i;

        for (i = 0; (name = util_nextword(name, i, names)); i++)
            run_send(name, localpath, remotepath);

        return EXIT_SUCCESS;

    }

    return error_missing();

}

static int parse_shell(int argc, char **argv)
{

    char *name = NULL;
    unsigned int argp = 0;
    unsigned int argi;

    for (argi = 0; argi < argc; argi++)
    {

        char *arg = argv[argi];

        if (arg[0] == '-')
        {

            return error_flag_unrecognized(arg);

        }

        else
        {

            switch (argp++)
            {

            case 0:
                name = assert_print(arg);

                break;

            default:
                return error_toomany();

            }

        }

    }

    if (name)
    {

        struct remote remote;

        remote_init(&remote, name);

        if (remote_load(&remote))
            return error_remote_load(name);

        if (remote_init_optional(&remote))
            return error_remote_init(name);

        if (ssh_connect(&remote))
            return error("Could not connect to remote '%s'.", remote.name);

        if (ssh_shell(&remote))
            return error("Could not open shell on remote '%s'.", remote.name);

        if (ssh_disconnect(&remote))
            return error("Could not disconnect from remote '%s'.", remote.name);

        return EXIT_SUCCESS;

    }

    return error_missing();

}

static int parse_version(int argc, char **argv)
{

    unsigned int argi;

    for (argi = 0; argi < argc; argi++)
    {

        char *arg = argv[argi];

        if (arg[0] == '-')
            return error_flag_unrecognized(arg);
        else
            return error_toomany();

    }

    {

        printf("%s version %s\n", CONFIG_PROGNAME, CONFIG_VERSION);

        return EXIT_SUCCESS;

    }

    return error_missing();

}

int main(int argc, char **argv)
{

    static struct command commands[] = {
        {"add", parse_add, "add <name> <hostname>", NULL, 1},
        {"config", parse_config, "config <namelist>", "List of keys:\n    name hostname port username password privatekey publickey tags\n", 1},
        {"config", parse_config, "config <namelist> <key>", "List of keys:\n    name hostname port username password privatekey publickey tags\n", 1},
        {"config", parse_config, "config <namelist> <key> <value>", "List of keys:\n    name hostname port username password privatekey publickey tags\n", 1},
        {"exec", parse_exec, "exec [-n] [-s] [-w] <namelist> <command>", "Args:\n    -n  No fork\n    -s  Sequential runs\n    -w  Wait for completion\n", 1},
        {"init", parse_init, "init", NULL, 0},
        {"list", parse_list, "list [-t <tags>]", NULL, 1},
        {"log", parse_log, "log [-c <count>] [-s <skip>]", "Args:\n    -c  Number of entries\n    -s  Skip entries\n", 1},
        {"log", parse_log, "log <refspec>", "Args:\n    -e  Show stderr\n", 1},
        {"log", parse_log, "log [-e] <refspec> <run>", "Args:\n    -e  Show stderr\n", 1},
        {"remove", parse_remove, "remove <namelist>", NULL, 1},
        {"send", parse_send, "send <namelist> <localpath> <remotepath>", NULL, 1},
        {"shell", parse_shell, "shell <name>", NULL, 1},
        {"version", parse_version, "version", NULL, 0},
        {0}
    };

    return assert_args(commands, argc - 1, argv + 1);

}

