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
#include "error.h"
#include "util.h"
#include "ini.h"
#include "log.h"
#include "run.h"
#include "remote.h"
#include "event.h"

struct command
{

    char *name;
    int (*parse)(int argc, char **argv);
    char *usage;
    char *description;
    unsigned int needconfig;

};

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

    return error_arg_invalid(argv[0]);

}

static char *assert_alpha(char *arg)
{

    util_trim(arg);

    if (!util_assert_alpha(arg))
        exit(error_arg_parse(arg, "alpha"));

    return arg;

}

static char *assert_digit(char *arg)
{

    util_trim(arg);

    if (!util_assert_digit(arg))
        exit(error_arg_parse(arg, "digit"));

    return arg;

}

static char *assert_print(char *arg)
{

    util_trim(arg);

    if (!util_assert_print(arg))
        exit(error_arg_parse(arg, "print"));

    return arg;

}

static char *assert_list(char *arg)
{

    util_trim(arg);
    util_strip(arg);

    if (!util_assert_printspace(arg))
        exit(error_arg_parse(arg, "printspace"));

    return arg;

}

static int run_exec(struct log *log, unsigned int pid, unsigned int index, char *name, char *command)
{

    struct remote remote;
    struct run run;

    remote_init(&remote, name);
    run_init(&run, index);

    if (remote_load(&remote))
        return error_remote_load(remote.name);

    if (remote_prepare(&remote))
        return error_remote_prepare(remote.name);

    if (run_prepare(&run, log))
        return error_run_prepare(run.index);

    if (run_update_remote(&run, log, remote.name))
        error_run_update(run.index, "remote");

    if (run_update_pid(&run, log, pid))
        error_run_update(run.index, "pid");

    if (run_update_status(&run, log, RUN_STATUS_PENDING))
        error_run_update(run.index, "status");

    if (!run_open(&run, log))
    {

        event_start(remote.name, run.index);

        if (!remote_connect(&remote))
        {

            int rc = remote_exec(&remote, command, run.stdoutfd, run.stderrfd);

            if (run_update_pid(&run, log, 0))
                error_run_update(run.index, "pid");

            if (rc == 0)
            {

                if (run_update_status(&run, log, RUN_STATUS_PASSED))
                    error_run_update(run.index, "status");

                log->passed++;
                log->complete++;

                if (log_update(log))
                    error("Could not update log.");

            }

            else
            {

                if (run_update_status(&run, log, RUN_STATUS_FAILED))
                    error_run_update(run.index, "status");

                log->failed++;
                log->complete++;

                if (log_update(log))
                    error("Could not update log.");

            }

            if (remote_disconnect(&remote))
                error_remote_disconnect(remote.name);

        }

        else
        {

            error_remote_connect(remote.name);

        }

        event_stop(remote.name, run.index);

        if (run_close(&run))
            error_run_close(run.index);

    }

    else
    {

        error_run_open(run.index);

    }

    return 0;

}

static int run_send(char *name, char *localpath, char *remotepath)
{

    struct remote remote;

    remote_init(&remote, name);

    if (remote_load(&remote))
        return error_remote_load(remote.name);

    if (remote_prepare(&remote))
        return error_remote_prepare(remote.name);

    if (!remote_connect(&remote))
    {

        int rc = remote_send(&remote, localpath, remotepath);

        if (rc == 0)
            event_send(remote.name);
        else
            error("Could not send file.");

        if (remote_disconnect(&remote))
            error_remote_disconnect(remote.name);

    }

    else
    {

        error_remote_connect(remote.name);

    }

    return 0;

}

static int parse_add(int argc, char **argv)
{

    char *hostname = NULL;
    char *name = NULL;
    char *type = "ssh";
    unsigned int argp = 0;
    unsigned int argi;

    for (argi = 0; argi < argc; argi++)
    {

        char *arg = argv[argi];

        if (arg[0] == '-')
        {

            switch (arg[1])
            {

            case 'h':
                hostname = assert_print(argv[++argi]);

                break;

            case 't':
                type = assert_print(argv[++argi]);

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

        if (type)
            remote_set_value(&remote, REMOTE_TYPE, type);

        if (hostname)
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

    unsigned int delete = 0;
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

            switch (arg[1])
            {

            case 'd':
                delete = 1;

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

            if (remote_set_value(&remote, keyhash, value) != keyhash)
                return error("Could not configure remote '%s' to set '%s' to '%s'.", remote.name, key, value);

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

            if (delete)
            {

                if (remote_set_value(&remote, keyhash, NULL) != keyhash)
                    return error("Could not configure remote '%s' to remove '%s'.", remote.name, key);

                if (remote_save(&remote))
                    return error_remote_save(name);

            }

            else
            {

                value = remote_get_value(&remote, keyhash);

                printf("%s: %s\n", remote.name, value);

            }

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

            if (remote.type)
                printf("type=%s\n", remote.type);

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

        struct log log;
        unsigned int names = util_split(name);

        log_init(&log, names);
        event_begin(log.id);

        if (log_prepare(&log))
            return error("Could not prepare log.");

        if (log_add(&log))
            return error("Could not add log.");

        if (nofork)
        {

            unsigned int i;

            for (i = 0; (name = util_nextword(name, i, names)); i++)
                run_exec(&log, 0, i, name, command);

        }

        else
        {

            unsigned int i;
            int status = 0;

            for (i = 0; (name = util_nextword(name, i, names)); i++)
            {

                pid_t pid = fork();

                if (pid == 0)
                    return run_exec(&log, getpid(), i, name, command);

                if (doseq)
                    waitpid(pid, &status, 0);

            }

            if (dowait)
                while (wait(&status) > 0);

        }

        event_end(log.id);

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

        char *hooks[] = {"begin", "end", "start", "stop", "send", 0};
        struct config_core core;
        char path[BUFSIZ];
        unsigned int i;
        int fd;

        if (mkdir(CONFIG_ROOT, 0775) < 0)
            return error("Already initialized.");

        if (!config_init())
            return error_init();

        core.version = CONFIG_VERSION;

        config_save(&core);
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

        struct log log;
        unsigned int r = strtoul(run, NULL, 10);

        if (log_open(&log) < 0)
            return error("Unable to open log.");

        if (log_find(&log, id))
            log_printstd(&log, r, descriptor);

        if (log_close(&log) < 0)
            return error("Unable to close log.");

        return EXIT_SUCCESS;

    }

    else if (id)
    {

        struct log log;

        if (log_open(&log) < 0)
            return error("Unable to open log.");

        if (log_find(&log, id))
            log_print(&log);

        if (log_close(&log) < 0)
            return error("Unable to close log.");

        return EXIT_SUCCESS;

    }

    else
    {

        struct log log;
        unsigned int s = strtoul(skip, NULL, 10);
        unsigned int c = strtoul(count, NULL, 10);
        unsigned int n;

        if (log_open(&log) < 0)
            return error("Unable to open log.");

        for (n = 1; log_readprev(&log); n++)
        {

            if (n > s)
                log_print(&log);

            if (c && n - s == c)
                break;

        }

        if (log_close(&log) < 0)
            return error("Unable to close log.");

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
    char *type = "vt102";
    unsigned int argp = 0;
    unsigned int argi;

    for (argi = 0; argi < argc; argi++)
    {

        char *arg = argv[argi];

        if (arg[0] == '-')
        {

            switch (arg[1])
            {

            case 't':
                type = assert_print(argv[++argi]);

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

        if (remote_prepare(&remote))
            return error_remote_prepare(name);

        if (!remote_connect(&remote))
        {

            if (remote_shell(&remote, type))
                error("Could not open shell of type '%s' on remote '%s'.", type, remote.name);

            if (remote_disconnect(&remote))
                error_remote_disconnect(remote.name);

        }

        else
        {

            return error_remote_connect(remote.name);

        }

        return EXIT_SUCCESS;

    }

    return error_missing();

}

static int parse_stop(int argc, char **argv)
{

    char *id = NULL;
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
                id = assert_print(arg);

                break;

            default:
                return error_toomany();

            }

        }

    }

    if (id)
    {

        struct log log;
        unsigned int i;

        if (log_open(&log) < 0)
            return error("Unable to open log.");

        if (!log_find(&log, id))
            return error("Unable to find log.");

        if (log_close(&log) < 0)
            return error("Unable to close log.");

        for (i = 0; i < log.total; i++)
        {

            struct run run;
            int pid;

            run_init(&run, i);

            pid = run_get_pid(&run, &log);

            if (pid >= 0)
            {

                kill(pid, SIGTERM);

                if (run_update_pid(&run, &log, 0))
                    error_run_update(run.index, "pid");

                if (run_update_status(&run, &log, RUN_STATUS_ABORTED))
                    error_run_update(run.index, "status");

                log.aborted++;
                log.complete++;

                if (log_update(&log))
                    error("Could not update log.");

            }

        }

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

static int parse_wait(int argc, char **argv)
{

    char *id = NULL;
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
                id = assert_print(arg);

                break;

            default:
                return error_toomany();

            }

        }

    }

    if (id)
    {

        struct log log;

        if (log_open(&log) < 0)
            return error("Unable to open log.");

        if (log_find(&log, id))
        {

            while (log.complete < log.total)
            {

                sleep(1);
                log_read(&log);

            }

        }

        if (log_close(&log) < 0)
            return error("Unable to close log.");

        return EXIT_SUCCESS;

    }

    return error_missing();

}

int main(int argc, char **argv)
{

    static struct command commands[] = {
        {"add", parse_add, "add [-h <hostname>] [-t <type>] <name>", "Args:\n    -h  Hostname\n    -t  Type\n", 1},
        {"config", parse_config, "config <namelist>", NULL, 1},
        {"config", parse_config, "config [-d] <namelist> <key>", "Args:\n    -d  Delete key\nList of keys:\n    name hostname port username password privatekey publickey tags\n", 1},
        {"config", parse_config, "config <namelist> <key> <value>", "List of keys:\n    name hostname port username password privatekey publickey tags\n", 1},
        {"exec", parse_exec, "exec [-n] [-s] [-w] <namelist> <command>", "Args:\n    -n  No fork\n    -s  Sequential runs\n    -w  Wait for completion\n", 1},
        {"init", parse_init, "init", NULL, 0},
        {"list", parse_list, "list [-t <tags>]", NULL, 1},
        {"log", parse_log, "log [-c <count>] [-s <skip>]", "Args:\n    -c  Number of entries\n    -s  Skip entries\n", 1},
        {"log", parse_log, "log <refspec>", "Args:\n    -e  Show stderr\n", 1},
        {"log", parse_log, "log [-e] <refspec> <run>", "Args:\n    -e  Show stderr\n", 1},
        {"remove", parse_remove, "remove <namelist>", NULL, 1},
        {"send", parse_send, "send <namelist> <localpath> <remotepath>", NULL, 1},
        {"shell", parse_shell, "shell [-t <type>] <name>", "Args:\n    -t  Terminal type (default: vt102)\n\n", 1},
        {"stop", parse_stop, "stop <refspec>", NULL, 1},
        {"version", parse_version, "version", NULL, 0},
        {"wait", parse_wait, "wait <refspec>", NULL, 1},
        {0}
    };

    return assert_args(commands, argc - 1, argv + 1);

}

