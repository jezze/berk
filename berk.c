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
#include "error.h"
#include "util.h"
#include "ini.h"
#include "log.h"
#include "run.h"
#include "remote.h"
#include "event.h"
#include "args.h"

struct command
{

    char *name;
    int (*parse)(int argc, char **argv);
    char *usage;
    unsigned int needconfig;

};

static void assert_args(struct args *args)
{

    if (args->state == ARGS_DONE)
        return;

    if (args->flag)
        exit(error_flag_unrecognized(args->value));

    exit(error("Unknown command"));

}

static unsigned int assert_command(char *arg)
{

    util_trim(arg);

    switch (arg[0])
    {

    case 'a':
        if (!strcmp(arg, "add"))
            return 1;

        break;

    case 'c':
        if (!strcmp(arg, "config"))
            return 2;

        break;

    case 'e':
        if (!strcmp(arg, "exec"))
            return 3;

        break;

    case 'h':
        if (!strcmp(arg, "help"))
            return 4;

        break;

    case 'i':
        if (!strcmp(arg, "init"))
            return 5;

        break;

    case 'l':
        if (!strcmp(arg, "list"))
            return 6;

        if (!strcmp(arg, "log"))
            return 7;

        break;

    case 'r':
        if (!strcmp(arg, "remove"))
            return 8;

        break;

    case 's':
        if (!strcmp(arg, "send"))
            return 9;

        if (!strcmp(arg, "shell"))
            return 10;

        if (!strcmp(arg, "stop"))
            return 11;

        break;

    case 'v':
        if (!strcmp(arg, "version"))
            return 12;

        break;

    case 'w':
        if (!strcmp(arg, "wait"))
            return 13;

        break;

    }

    exit(error_arg_parse(arg, "command"));

    return 0;

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

static void update(struct log *log)
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
        error("Could not update log.");

}

static int run_exec(char *id, unsigned int pid, unsigned int index, char *name, char *command)
{

    struct remote remote;
    struct run run;

    remote_init(&remote, name);
    run_init(&run, index);

    if (remote_load(&remote))
        return error_remote_load(remote.name);

    if (remote_prepare(&remote))
        return error_remote_prepare(remote.name);

    if (run_prepare(&run, id))
        return error_run_prepare(run.index);

    if (run_update_remote(&run, id, remote.name))
        error_run_update(run.index, "remote");

    if (run_update_pid(&run, id, pid))
        error_run_update(run.index, "pid");

    if (run_update_status(&run, id, RUN_STATUS_PENDING))
        error_run_update(run.index, "status");

    if (!run_open(&run, id))
    {

        event_start(remote.name, run.index);

        if (!remote_connect(&remote))
        {

            int rc = remote_exec(&remote, command, run.stdoutfd, run.stderrfd);

            if (run_update_pid(&run, id, 0))
                error_run_update(run.index, "pid");

            if (run_update_status(&run, id, rc == 0 ? RUN_STATUS_PASSED : RUN_STATUS_FAILED))
                error_run_update(run.index, "status");

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

static int command_add(struct args *args)
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
            switch (args->position)
            {

            case 1:
                name = assert_print(args->value);

                break;

            default:
                return error_toomany();

            }

            break;

        }

    }

    assert_args(args);

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

static int command_config(struct args *args)
{

    unsigned int delete = 0;
    char *value = NULL;
    char *name = NULL;
    char *key = NULL;

    args_setoptions(args, "d");

    while (args_next(args))
    {

        switch (args->flag)
        {

        case 'd':
            delete = 1;

            break;

        default:
            switch (args->position)
            {

            case 1:
                name = assert_list(args->value);

                break;

            case 2:
                key = assert_alpha(args->value);

                break;

            case 3:
                value = assert_print(args->value);

                break;

            default:
                return error_toomany();

            }

            break;

        }

    }

    assert_args(args);

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

static int command_exec(struct args *args)
{

    unsigned int doseq = 0;
    unsigned int dowait = 0;
    unsigned int nofork = 0;
    char *command = NULL;
    char *name = NULL;

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
                name = assert_list(args->value);

                break;

            case 2:
                command = args->value;

                break;

            default:
                return error_toomany();

            }

            break;

        }

    }

    assert_args(args);

    if (name && command)
    {

        unsigned int names = util_split(name);
        struct log log;

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
            {

                run_exec(log.id, 0, i, name, command);
                update(&log);

            }

        }

        else
        {

            unsigned int i;
            int status = 0;

            for (i = 0; (name = util_nextword(name, i, names)); i++)
            {

                pid_t pid = fork();

                if (pid == 0)
                {

                    int rc = run_exec(log.id, getpid(), i, name, command);

                    update(&log);

                    return rc;

                }

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

static int command_help(struct args *args)
{

    args_setoptions(args, 0);

    while (args_next(args))
        return error_toomany();

    assert_args(args);

    {

        printf("Usage: %s <command> [<args>]\n", args->argv[0]);
        printf("\n");
        printf("List of commands:\n");
        printf("    %s\n", "add [-h <hostname>] [-t <type>] <name>");
        printf("    %s\n", "config <namelist>");
        printf("    %s\n", "config [-d] <namelist> <key>");
        printf("    %s\n", "config <namelist> <key> <value>");
        printf("    %s\n", "exec [-n] [-s] [-w] <namelist> <command>");
        printf("    %s\n", "help");
        printf("    %s\n", "init");
        printf("    %s\n", "list [-t <tags>]");
        printf("    %s\n", "log [-c <count>] [-s <skip>]");
        printf("    %s\n", "log <refspec>");
        printf("    %s\n", "log [-e] <refspec> <run>");
        printf("    %s\n", "remove <namelist>");
        printf("    %s\n", "send <namelist> <localpath> <remotepath>");
        printf("    %s\n", "shell [-t <type>] <name>");
        printf("    %s\n", "stop <refspec>");
        printf("    %s\n", "version");
        printf("    %s\n", "wait <refspec>");

    }

    return EXIT_SUCCESS;

}

static int command_init(struct args *args)
{

    args_setoptions(args, 0);

    while (args_next(args))
        return error_toomany();

    assert_args(args);

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

static int command_list(struct args *args)
{

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
            return error_toomany();

        }

    }

    assert_args(args);

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

static int command_log(struct args *args)
{

    unsigned int descriptor = 1;
    char *count = "0";
    char *skip = "0";
    char *run = NULL;
    char *id = NULL;

    args_setoptions(args, "c:es:");

    while (args_next(args))
    {

        switch (args->flag)
        {

        case 'e':
            descriptor = 2;

            break;

        case 'c':
            count = assert_digit(args->value);

            break;

        case 's':
            skip = assert_digit(args->value);

            break;

        default:
            switch (args->position)
            {

            case 1:
                id = assert_print(args->value);

                break;

            case 2:
                run = assert_digit(args->value);

                break;

            default:
                return error_toomany();

            }

            break;

        }

    }

    assert_args(args);

    if (id && run)
    {

        unsigned int r = strtoul(run, NULL, 10);
        struct log log;

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

        unsigned int s = strtoul(skip, NULL, 10);
        unsigned int c = strtoul(count, NULL, 10);
        struct log log;
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

static int command_remove(struct args *args)
{

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
                name = assert_list(args->value);

                break;

            default:
                return error_toomany();

            }

            break;

        }

    }

    assert_args(args);

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

static int command_send(struct args *args)
{

    char *name = NULL;
    char *remotepath = NULL;
    char *localpath = NULL;

    args_setoptions(args, 0);

    while (args_next(args))
    {

        switch (args->flag)
        {

        default:
            switch (args->position)
            {

            case 1:
                name = assert_list(args->value);

                break;

            case 2:
                localpath = assert_print(args->value);

                break;

            case 3:
                remotepath = assert_print(args->value);

                break;

            default:
                return error_toomany();

            }

            break;

        }

    }

    assert_args(args);

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
            switch (args->position)
            {

            case 1:
                name = assert_print(args->value);

                break;

            default:
                return error_toomany();

            }

            break;

        }

    }

    assert_args(args);

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

static int command_stop(struct args *args)
{

    char *id = NULL;

    args_setoptions(args, 0);

    while (args_next(args))
    {

        switch (args->flag)
        {

        default:
            switch (args->position)
            {

            case 1:
                id = assert_print(args->value);

                break;

            default:
                return error_toomany();

            }

            break;

        }

    }

    assert_args(args);

    if (id)
    {

        unsigned int i;
        struct log log;

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

            pid = run_get_pid(&run, log.id);

            if (pid)
            {

                kill(pid, SIGTERM);

                if (run_update_pid(&run, log.id, 0))
                    error_run_update(run.index, "pid");

                if (run_update_status(&run, log.id, RUN_STATUS_ABORTED))
                    error_run_update(run.index, "status");

            }

        }

        update(&log);

        return EXIT_SUCCESS;

    }

    return error_missing();

}

static int command_version(struct args *args)
{

    args_setoptions(args, 0);

    while (args_next(args))
        return error_toomany();

    assert_args(args);

    printf("%s version %s\n", CONFIG_PROGNAME, CONFIG_VERSION);

    return EXIT_SUCCESS;

}

static int command_wait(struct args *args)
{

    char *id = NULL;

    args_setoptions(args, 0);

    while (args_next(args))
    {

        switch (args->flag)
        {

        default:
            switch (args->position)
            {

            case 1:
                id = assert_print(args->value);

                break;

            default:
                return error_toomany();

            }

            break;

        }

    }

    assert_args(args);

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

static int command(struct args *args)
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
                command = assert_command(args->value);

                switch (command)
                {

                case 1:
                case 2:
                case 3:
                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                case 11:
                case 13:
                    if (!config_init())
                        return error_init();

                    break;

                }

                switch (command)
                {

                case 1:
                    return command_add(args);

                case 2:
                    return command_config(args);

                case 3:
                    return command_exec(args);

                case 4:
                    return command_help(args);

                case 5:
                    return command_init(args);

                case 6:
                    return command_list(args);

                case 7:
                    return command_log(args);

                case 8:
                    return command_remove(args);

                case 9:
                    return command_send(args);

                case 10:
                    return command_shell(args);

                case 11:
                    return command_stop(args);

                case 12:
                    return command_version(args);

                case 13:
                    return command_wait(args);

                }

                break;

            default:
                return error_toomany();

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

    return command(&args);

}

