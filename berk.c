#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <libssh2.h>
#include "config.h"
#include "util.h"
#include "ini.h"
#include "log.h"
#include "run.h"
#include "remote.h"
#include "event.h"
#include "args.h"

#define ERROR_EXIST                     "Already initialized."
#define ERROR_INIT                      "Could not find '%s' directory."
#define ERROR_MKDIR                     "Could not create directory '%s'."
#define ERROR_OPENDIR                   "Could not open directory '%s'."
#define ERROR_HOOK_CREATE               "Could not create hook '%s'."
#define ERROR_LOG_OPEN                  "Could not open log."
#define ERROR_LOG_CLOSE                 "Could not close log."
#define ERROR_LOG_ADD                   "Could not add log."
#define ERROR_LOG_UPDATE                "Could not update log."
#define ERROR_LOG_PREPARE               "Could not prepare log."
#define ERROR_LOG_FIND                  "Could not find log '%s'."
#define ERROR_REMOTE_PREPARE            "Could not init remote '%s'."
#define ERROR_REMOTE_LOAD               "Could not load remote '%s'."
#define ERROR_REMOTE_SAVE               "Could not save remote '%s'."
#define ERROR_REMOTE_CONNECT            "Could not connect to remote '%s'."
#define ERROR_REMOTE_DISCONNECT         "Could not disconnect from remote '%s'."
#define ERROR_REMOTE_SET                "Could not configure remote '%s' to set '%s' to '%s'."
#define ERROR_REMOTE_SEND               "Could not send file."
#define ERROR_REMOTE_SHELL              "Could not open shell of type '%s' on remote '%s'."
#define ERROR_REMOTE_REMOVE             "Could not remove remote '%s'."
#define ERROR_RUN_PREPARE               "Could not prepare run '%u'."
#define ERROR_RUN_UPDATE                "Could not update run '%u' with type '%s'."
#define ERROR_RUN_OPEN                  "Could not open run '%u'."
#define ERROR_RUN_CLOSE                 "Could not close run '%u'."
#define ERROR_ARG_MANY                  "Too many arguments."
#define ERROR_ARG_COMMAND               "Unrecognized command '%s'."
#define ERROR_ARG_INVALID               "Invalid argument '%s'."
#define ERROR_ARG_PARSE                 "Could not parse '%s' as '%s'."
#define ERROR_ARG_FLAG                  "Unrecognized flag '%s'."
#define ERROR_ARG_UNKNOWN               "Argument parsing failed."

int error(char *format, ...)
{

    va_list args;

    va_start(args, format);
    fprintf(stderr, "%s: ", CONFIG_PROGNAME);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);

    return EXIT_FAILURE;

}

void panic(char *format, ...)
{

    va_list args;

    va_start(args, format);
    fprintf(stderr, "%s: ", CONFIG_PROGNAME);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(EXIT_FAILURE);

}

static void assert_args(struct args *args)
{

    if (args->state == ARGS_DONE)
        return;

    if (args->flag)
        panic(ERROR_ARG_FLAG, args->value);

    panic(ERROR_ARG_UNKNOWN);

}

static unsigned int get_command_config(char *arg)
{

    if (!strcmp(arg, "get"))
        return 1;

    if (!strcmp(arg, "list"))
        return 2;

    if (!strcmp(arg, "set"))
        return 3;

    if (!strcmp(arg, "unset"))
        return 4;

    panic(ERROR_ARG_COMMAND, arg);

    return 0;

}

static unsigned int get_command_remote(char *arg)
{

    if (!strcmp(arg, "add"))
        return 1;

    if (!strcmp(arg, "remove"))
        return 2;

    panic(ERROR_ARG_COMMAND, arg);

    return 0;

}

static unsigned int get_command_main(char *arg)
{

    if (!strcmp(arg, "config"))
        return 1;

    if (!strcmp(arg, "exec"))
        return 2;

    if (!strcmp(arg, "help"))
        return 3;

    if (!strcmp(arg, "init"))
        return 4;

    if (!strcmp(arg, "log"))
        return 5;

    if (!strcmp(arg, "remote"))
        return 6;

    if (!strcmp(arg, "send"))
        return 7;

    if (!strcmp(arg, "shell"))
        return 8;

    if (!strcmp(arg, "show"))
        return 9;

    if (!strcmp(arg, "stop"))
        return 10;

    if (!strcmp(arg, "version"))
        return 11;

    if (!strcmp(arg, "wait"))
        return 12;

    panic(ERROR_ARG_COMMAND, arg);

    return 0;

}

static char *assert_alpha(char *arg)
{

    if (!util_assert_alpha(arg))
        panic(ERROR_ARG_PARSE, arg, "alpha");

    return arg;

}

static char *assert_digit(char *arg)
{

    if (!util_assert_digit(arg))
        panic(ERROR_ARG_PARSE, arg, "digit");

    return arg;

}

static char *assert_print(char *arg)
{

    if (!util_assert_print(arg))
        panic(ERROR_ARG_PARSE, arg, "print");

    return arg;

}

static void updatelog(struct log *log)
{

    unsigned int i;

    log->complete = 0;
    log->aborted = 0;
    log->passed = 0;
    log->failed = 0;

    for (i = 0; i < log->total; i++)
    {

        struct run run;
        unsigned int pid;
        unsigned int status;

        run_init(&run, i);

        pid = run_get_pid(&run, log->id);

        if (pid == 0)
            log->complete++;

        status = run_get_status(&run, log->id);

        switch (status)
        {

        case RUN_STATUS_ABORTED:
            log->aborted++;

            break;

        case RUN_STATUS_PASSED:
            log->passed++;

            break;

        case RUN_STATUS_FAILED:
            log->failed++;

            break;

        }

    }

    if (log_update(log))
        panic(ERROR_LOG_UPDATE);

}

static int execute(char *id, unsigned int pid, unsigned int index, char *name, char *command)
{

    struct remote remote;
    struct run run;
    int rc;

    remote_init(&remote, name);
    run_init(&run, index);

    if (remote_load(&remote))
        panic(ERROR_REMOTE_LOAD, remote.name);

    if (remote_prepare(&remote))
        panic(ERROR_REMOTE_PREPARE, remote.name);

    if (run_prepare(&run, id))
        panic(ERROR_RUN_PREPARE, run.index);

    if (run_update_remote(&run, id, remote.name))
        panic(ERROR_RUN_UPDATE, run.index, "remote");

    if (run_update_pid(&run, id, pid))
        panic(ERROR_RUN_UPDATE, run.index, "pid");

    if (run_update_status(&run, id, RUN_STATUS_PENDING))
        panic(ERROR_RUN_UPDATE, run.index, "status");

    if (run_open(&run, id))
        panic(ERROR_RUN_OPEN, run.index);

    event_start(remote.name, run.index);

    if (remote_connect(&remote))
        panic(ERROR_REMOTE_CONNECT, remote.name);

    rc = remote_exec(&remote, command, run.stdoutfd, run.stderrfd);

    if (run_update_pid(&run, id, 0))
        panic(ERROR_RUN_UPDATE, run.index, "pid");

    if (run_update_status(&run, id, rc == 0 ? RUN_STATUS_PASSED : RUN_STATUS_FAILED))
        panic(ERROR_RUN_UPDATE, run.index, "status");

    if (remote_disconnect(&remote))
        panic(ERROR_REMOTE_DISCONNECT, remote.name);

    event_stop(remote.name, run.index);

    if (run_close(&run))
        panic(ERROR_RUN_CLOSE, run.index);

    return 0;

}

static void do_config_get(char *name, char *key)
{

    unsigned int keyhash = util_hash(key);
    struct remote remote;
    char *value;

    remote_init(&remote, name);

    if (remote_load(&remote))
        panic(ERROR_REMOTE_LOAD, name);

    value = remote_get_value(&remote, keyhash);

    printf("%s: %s\n", remote.name, value);

}

static void do_config_list(char *name)
{

    struct remote remote;

    remote_init(&remote, name);

    if (remote_load(&remote))
        panic(ERROR_REMOTE_LOAD, name);

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

static void do_config_set(char *name, char *key, char *value)
{

    unsigned int keyhash = util_hash(key);
    struct remote remote;

    remote_init(&remote, name);

    if (remote_load(&remote))
        panic(ERROR_REMOTE_LOAD, name);

    if (remote_set_value(&remote, keyhash, value) != keyhash)
        panic(ERROR_REMOTE_SET, remote.name, key, value);

    if (remote_save(&remote))
        panic(ERROR_REMOTE_SAVE, name);

}

static void do_config_unset(char *name, char *key)
{

    unsigned int keyhash = util_hash(key);
    struct remote remote;

    remote_init(&remote, name);

    if (remote_load(&remote))
        panic(ERROR_REMOTE_LOAD, name);

    if (remote_set_value(&remote, keyhash, NULL) != keyhash)
        panic(ERROR_REMOTE_SET, remote.name, key, "NULL");

    if (remote_save(&remote))
        panic(ERROR_REMOTE_SAVE, name);

}

static void do_exec(char *command, unsigned int nofork, unsigned int doseq, unsigned int dowait, unsigned int names, char **name)
{

    struct log log;

    log_init(&log, names);

    if (log_prepare(&log))
        panic(ERROR_LOG_PREPARE);

    if (log_add(&log))
        panic(ERROR_LOG_ADD);

    printf("%s\n", log.id);
    event_begin(log.id);

    if (nofork)
    {

        unsigned int i;

        for (i = 0; i < names; i++)
        {

            execute(log.id, 0, i, name[i], command);
            updatelog(&log);

        }

    }

    else
    {

        unsigned int i;
        int status = 0;

        for (i = 0; i < names; i++)
        {

            pid_t pid = fork();

            if (pid == 0)
            {

                execute(log.id, getpid(), i, name[i], command);
                updatelog(&log);

            }

            if (doseq)
                waitpid(pid, &status, 0);

        }

        if (dowait)
            while (wait(&status) > 0);

    }

    event_end(log.id);

}

static void do_init(void)
{

    char *hooks[] = {"begin", "end", "start", "stop", "send", 0};
    struct config_core core;
    char path[BUFSIZ];
    unsigned int i;
    int fd;

    if (mkdir(CONFIG_ROOT, 0775) < 0)
        panic(ERROR_EXIST);

    if (!config_init())
        panic(ERROR_INIT, CONFIG_ROOT);

    core.version = CONFIG_VERSION;

    config_save(&core);
    config_get_path(path, BUFSIZ, CONFIG_HOOKS);

    if (mkdir(path, 0775) < 0)
        panic(ERROR_MKDIR, CONFIG_HOOKS);

    for (i = 0; hooks[i]; i++)
    {

        char buffer[BUFSIZ];

        snprintf(buffer, BUFSIZ, "%s.sample", hooks[i]);
        config_get_subpath(path, BUFSIZ, CONFIG_HOOKS, buffer);

        fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0755);

        if (fd < 0)
            panic(ERROR_HOOK_CREATE, hooks[i]);

        dprintf(fd, "#!/bin/sh\n\n# To enable this hook, rename it to \"%s\".\n", hooks[i]);
        close(fd);

    }

    printf("Initialized %s in '%s'.\n", CONFIG_PROGNAME, CONFIG_ROOT);

}

static void do_log(char *count, char *skip)
{

    unsigned int c = strtoul(count, NULL, 10);
    unsigned int s = strtoul(skip, NULL, 10);
    struct log log;
    unsigned int n;

    if (log_open(&log) < 0)
        panic(ERROR_LOG_OPEN);

    for (n = 1; log_readprev(&log); n++)
    {

        if (n > s)
            log_print(&log);

        if (c && n - s == c)
            break;

    }

    if (log_close(&log) < 0)
        panic(ERROR_LOG_CLOSE);

}

static void do_remote_add(char *name, char *type, char *hostname)
{

    struct remote remote;

    remote_init(&remote, name);

    if (type)
        remote_set_value(&remote, REMOTE_TYPE, type);

    if (hostname)
        remote_set_value(&remote, REMOTE_HOSTNAME, hostname);

    if (remote_save(&remote))
        panic(ERROR_REMOTE_SAVE, name);

    printf("Remote '%s' added.\n", name);

}

static void do_remote_remove(char *name)
{

    struct remote remote;

    remote_init(&remote, name);

    if (remote_load(&remote))
        panic(ERROR_REMOTE_LOAD, name);

    if (remote_remove(&remote))
        panic(ERROR_REMOTE_REMOVE, remote.name);

    printf("Remote '%s' removed.\n", remote.name);

}

static void do_remote(char *tags)
{

    struct dirent *entry;
    char path[BUFSIZ];
    DIR *dir;

    config_get_path(path, BUFSIZ, CONFIG_REMOTES);

    dir = opendir(path);

    if (dir == NULL)
        panic(ERROR_OPENDIR, path);

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

}

static void do_send(char *name, char *localpath, char *remotepath)
{

    struct remote remote;
    int rc;

    remote_init(&remote, name);

    if (remote_load(&remote))
        panic(ERROR_REMOTE_LOAD, remote.name);

    if (remote_prepare(&remote))
        panic(ERROR_REMOTE_PREPARE, remote.name);

    if (remote_connect(&remote))
        panic(ERROR_REMOTE_CONNECT, remote.name);

    rc = remote_send(&remote, localpath, remotepath);

    if (rc == 0)
        event_send(remote.name);
    else
        panic(ERROR_REMOTE_SEND);

    if (remote_disconnect(&remote))
        panic(ERROR_REMOTE_DISCONNECT, remote.name);

}

static void do_shell(char *name, char *type)
{

    struct remote remote;

    remote_init(&remote, name);

    if (remote_load(&remote))
        panic(ERROR_REMOTE_LOAD, name);

    if (remote_prepare(&remote))
        panic(ERROR_REMOTE_PREPARE, name);

    if (remote_connect(&remote))
        panic(ERROR_REMOTE_CONNECT, remote.name);

    if (remote_shell(&remote, type))
        panic(ERROR_REMOTE_SHELL, type, remote.name);

    if (remote_disconnect(&remote))
        panic(ERROR_REMOTE_DISCONNECT, remote.name);

}

static void do_show(char *id, char *run, unsigned int descriptor)
{

    unsigned int r = strtoul(run, NULL, 10);
    struct log log;

    if (log_open(&log) < 0)
        panic(ERROR_LOG_OPEN);

    if (!log_find(&log, id))
        panic(ERROR_LOG_FIND, id);

    switch (descriptor)
    {

    case 0:
        log_print(&log);

        break;

    case 1:
    case 2:
        log_printstd(&log, r, descriptor);

        break;

    }

    if (log_close(&log) < 0)
        panic(ERROR_LOG_CLOSE);

}

static void do_stop(char *id)
{

    unsigned int i;
    struct log log;

    if (log_open(&log) < 0)
        panic(ERROR_LOG_OPEN);

    if (!log_find(&log, id))
        panic(ERROR_LOG_FIND, id);

    if (log_close(&log) < 0)
        panic(ERROR_LOG_CLOSE);

    for (i = 0; i < log.total; i++)
    {

        struct run run;
        int pid;

        run_init(&run, i);

        pid = run_get_pid(&run, log.id);

        if (pid)
        {

            kill(pid, SIGTERM);

            if (run_update_pid(&run, log.id, 0))
                panic(ERROR_RUN_UPDATE, run.index, "pid");

            if (run_update_status(&run, log.id, RUN_STATUS_ABORTED))
                panic(ERROR_RUN_UPDATE, run.index, "status");

        }

    }

    updatelog(&log);

}

static void do_wait(char *id)
{

    struct log log;

    if (log_open(&log) < 0)
        panic(ERROR_LOG_OPEN);

    if (!log_find(&log, id))
        panic(ERROR_LOG_FIND, id);

    while (log.complete < log.total)
    {

        sleep(1);
        log_read(&log);

    }

    if (log_close(&log) < 0)
        panic(ERROR_LOG_CLOSE);

}

static int command_config_get(struct args *args)
{

    char *name = NULL;
    char *key = NULL;

    args_setoptions(args, 0);

    while (args_next(args))
    {

        switch (args->flag)
        {

        default:
            switch (args->position)
            {

            case 1:
                key = assert_alpha(args->value);

                break;

            default:
                name = assert_print(args->value);

                if (name)
                    do_config_get(name, key);

                break;

            }

        }

    }

    assert_args(args);

    return EXIT_SUCCESS;

}

static int command_config_list(struct args *args)
{

    char *name = NULL;

    args_setoptions(args, 0);

    while (args_next(args))
    {

        switch (args->flag)
        {

        default:
            name = assert_print(args->value);

            if (name)
                do_config_list(name);

            break;

        }

    }

    assert_args(args);

    return EXIT_SUCCESS;

}

static int command_config_set(struct args *args)
{

    char *value = NULL;
    char *name = NULL;
    char *key = NULL;

    args_setoptions(args, 0);

    while (args_next(args))
    {

        switch (args->flag)
        {

        default:
            switch (args->position)
            {

            case 1:
                key = assert_alpha(args->value);

                break;

            case 2:
                value = assert_print(args->value);

                break;

            default:
                name = assert_print(args->value);

                if (name)
                    do_config_set(name, key, value);

                break;

            }

        }

    }

    assert_args(args);

    return EXIT_SUCCESS;

}

static int command_config_unset(struct args *args)
{

    char *name = NULL;
    char *key = NULL;

    args_setoptions(args, 0);

    while (args_next(args))
    {

        switch (args->flag)
        {

        default:
            switch (args->position)
            {

            case 1:
                key = assert_alpha(args->value);

                break;

            default:
                name = assert_print(args->value);

                if (name)
                    do_config_unset(name, key);

                break;

            }

        }

    }

    assert_args(args);

    return EXIT_SUCCESS;

}

static int command_config(struct args *args)
{

    unsigned int command = 0;

    args_setoptions(args, 0);

    while (args_next(args))
    {

        switch (args->flag)
        {

        default:
            switch (args->position)
            {

            case 1:
                command = get_command_config(args->value);

                switch (command)
                {

                case 1:
                    return command_config_get(args);

                case 2:
                    return command_config_list(args);

                case 3:
                    return command_config_set(args);

                case 4:
                    return command_config_unset(args);
 
                }

            }

            break;

        }

    }

    assert_args(args);

    return EXIT_SUCCESS;

}

static int command_exec(struct args *args)
{

    unsigned int doseq = 0;
    unsigned int dowait = 0;
    unsigned int nofork = 0;
    char *command = NULL;

    args_setoptions(args, "nsw");

    while (args_next(args))
    {

        switch (args->flag)
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
            switch (args->position)
            {

            case 1:
                command = args->value;

                break;

            default:
                do_exec(command, nofork, doseq, dowait, args->argc - args->index + 1, args->argv + args->index - 1);

                return EXIT_SUCCESS;

            }

            break;

        }

    }

    assert_args(args);

    return EXIT_SUCCESS;

}

static int command_help(struct args *args)
{

    args_setoptions(args, 0);

    while (args_next(args))
        panic(ERROR_ARG_MANY);

    assert_args(args);
    printf("Usage: %s <command> [<args>]\n", args->argv[0]);
    printf("\n");
    printf("List of commands:\n");
    printf("    %s\n", "config get <key> <remote> [<remote>...]");
    printf("    %s\n", "config list <remote> [<remote>...]");
    printf("    %s\n", "config set <key> <value> <remote> [<remote>...]");
    printf("    %s\n", "config unset <key> <remote> [<remote>...]");
    printf("    %s\n", "exec [-n] [-s] [-w] <command> <remote> [<remote>...]");
    printf("    %s\n", "help");
    printf("    %s\n", "init");
    printf("    %s\n", "log [-c <count>] [-s <skip>]");
    printf("    %s\n", "remote [-t <tags>]");
    printf("    %s\n", "remote add [-h <hostname>] [-t <type>] <name>");
    printf("    %s\n", "remote remove <remote> [<remote>...]");
    printf("    %s\n", "send <localpath> <remotepath> <remote> [<remote>...]");
    printf("    %s\n", "shell [-t <type>] <remote>");
    printf("    %s\n", "show [-e] [-i <refspec>] [-o] [-r <run>]");
    printf("    %s\n", "stop [<refspec>...]");
    printf("    %s\n", "version");
    printf("    %s\n", "wait [<refspec>...]");

    return EXIT_SUCCESS;

}

static int command_init(struct args *args)
{

    args_setoptions(args, 0);

    while (args_next(args))
        panic(ERROR_ARG_MANY);

    assert_args(args);
    do_init();

    return EXIT_SUCCESS;

}

static int command_log(struct args *args)
{

    char *count = "0";
    char *skip = "0";

    args_setoptions(args, "c:es:");

    while (args_next(args))
    {

        switch (args->flag)
        {

        case 'c':
            count = assert_digit(args->value);

            break;

        case 's':
            skip = assert_digit(args->value);

            break;

        default:
            panic(ERROR_ARG_MANY);

        }

    }

    assert_args(args);
    do_log(count, skip);

    return EXIT_SUCCESS;

}

static int command_remote_add(struct args *args)
{

    char *hostname = NULL;
    char *name = NULL;
    char *type = "ssh";

    args_setoptions(args, "h:t:");

    while (args_next(args))
    {

        switch (args->flag)
        {

        case 'h':
            hostname = assert_print(args->value);

            break;

        case 't':
            type = assert_print(args->value);

            break;

        default:
            name = assert_print(args->value);

            if (name)
                do_remote_add(name, type, hostname);

            break;

        }

    }

    assert_args(args);

    return EXIT_SUCCESS;

}

static int command_remote_remove(struct args *args)
{

    char *name = NULL;

    args_setoptions(args, 0);

    while (args_next(args))
    {

        switch (args->flag)
        {

        default:
            name = assert_print(args->value);

            if (name)
                do_remote_remove(name);

            break;

        }

    }

    assert_args(args);

    return EXIT_SUCCESS;

}

static int command_remote(struct args *args)
{

    unsigned int command = 0;
    char *tags = NULL;

    args_setoptions(args, "t:");

    while (args_next(args))
    {

        switch (args->flag)
        {

        case 't':
            tags = assert_print(args->value);

            break;

        default:
            switch (args->position)
            {

            case 1:
                command = get_command_remote(args->value);

                switch (command)
                {

                case 1:
                    return command_remote_add(args);

                case 2:
                    return command_remote_remove(args);
 
                }

            }

            break;

        }

    }

    assert_args(args);
    do_remote(tags);

    return EXIT_SUCCESS;

}

static int command_send(struct args *args)
{

    char *remotepath = NULL;
    char *localpath = NULL;
    char *name = NULL;

    args_setoptions(args, 0);

    while (args_next(args))
    {

        switch (args->flag)
        {

        default:
            switch (args->position)
            {

            case 1:
                localpath = assert_print(args->value);

                break;

            case 2:
                remotepath = assert_print(args->value);

                break;

            default:
                name = assert_print(args->value);

                if (name && localpath && remotepath)
                    do_send(name, localpath, remotepath);

                break;

            }

            break;

        }

    }

    assert_args(args);

    return EXIT_SUCCESS;

}

static int command_shell(struct args *args)
{

    char *name = NULL;
    char *type = "vt102";

    args_setoptions(args, "t:");

    while (args_next(args))
    {

        switch (args->flag)
        {

        case 't':
            type = assert_print(args->value);

            break;

        default:
            name = assert_print(args->value);

            if (name)
                do_shell(name, type);

            break;

        }

    }

    assert_args(args);

    return EXIT_SUCCESS;

}

static int command_show(struct args *args)
{

    unsigned int descriptor = 0;
    char *run = "0";
    char *id = "HEAD";

    args_setoptions(args, "ei:or:");

    while (args_next(args))
    {

        switch (args->flag)
        {

        case 'o':
            descriptor = 1;

            break;

        case 'e':
            descriptor = 2;

            break;

        case 'i':
            id = assert_print(args->value);

            break;

        case 'r':
            run = assert_digit(args->value);

            break;

        default:
            panic(ERROR_ARG_MANY);

        }

    }

    assert_args(args);
    do_show(id, run, descriptor);

    return EXIT_SUCCESS;

}

static int command_stop(struct args *args)
{

    char *id = "HEAD";

    args_setoptions(args, 0);

    while (args_next(args))
    {

        switch (args->flag)
        {

        default:
            id = assert_print(args->value);

            if (id)
                do_stop(id);

            id = NULL;

            break;

        }

    }

    assert_args(args);

    if (id)
        do_stop(id);

    return EXIT_SUCCESS;

}

static int command_version(struct args *args)
{

    args_setoptions(args, 0);

    while (args_next(args))
        panic(ERROR_ARG_MANY);

    assert_args(args);

    printf("%s version %s\n", CONFIG_PROGNAME, CONFIG_VERSION);

    return EXIT_SUCCESS;

}

static int command_wait(struct args *args)
{

    char *id = "HEAD";

    args_setoptions(args, 0);

    while (args_next(args))
    {

        switch (args->flag)
        {

        default:
            id = assert_print(args->value);

            if (id)
                do_wait(id);

            id = NULL;

            break;

        }

    }

    assert_args(args);

    if (id)
        do_wait(id);

    return EXIT_SUCCESS;

}

static int command_main(struct args *args)
{

    unsigned int command = 0;

    args_setoptions(args, "h");

    while (args_next(args))
    {

        switch (args->flag)
        {

        default:
            switch (args->position)
            {

            case 1:
                command = get_command_main(args->value);

                switch (command)
                {

                case 3:
                case 4:
                case 11:
                    break;

                default:
                    if (!config_init())
                        panic(ERROR_INIT, CONFIG_ROOT);

                    break;

                }

                switch (command)
                {

                case 1:
                    return command_config(args);

                case 2:
                    return command_exec(args);

                case 3:
                    return command_help(args);

                case 4:
                    return command_init(args);

                case 5:
                    return command_log(args);

                case 6:
                    return command_remote(args);

                case 7:
                    return command_send(args);

                case 8:
                    return command_shell(args);

                case 9:
                    return command_show(args);

                case 10:
                    return command_stop(args);

                case 11:
                    return command_version(args);

                case 12:
                    return command_wait(args);

                }

                break;

            default:
                panic(ERROR_ARG_MANY);

            }

        }

    }

    return command_help(args);

}

int main(int argc, char **argv)
{

    struct args args;

    args_init(&args, argc, argv);
    args_next(&args);

    return command_main(&args);

}

