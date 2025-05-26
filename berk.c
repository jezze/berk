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
#include "log.h"
#include "event.h"
#include "ssh.h"

#define LOG_ENTRYSIZE 72

struct command
{

    char *name;
    int (*parse)(int argc, char **argv);
    char *usage;
    char *description;

};

static int error_init(void)
{

    return util_error("Could not find '%s' directory.", CONFIG_ROOT);

}

static int error_path(void)
{

    return util_error("Could not get path.");

}

static int error_remote_init(char *name)
{

    return util_error("Could not init remote '%s'.", name);

}

static int error_remote_load(char *name)
{

    return util_error("Could not load remote '%s'.", name);

}

static int error_remote_save(char *name)
{

    return util_error("Could not save remote '%s'.", name);

}

static int error_missing(void)
{

    return util_error("Missing arguments.");

}

static int error_toomany(void)
{

    return util_error("Too many arguments.");

}

static int error_flag_unrecognized(char *arg)
{

    return util_error("Unrecognized flag '%s'.", arg);

}

static int assert_args(struct command *commands, int argc, char **argv)
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

static char *assert_alpha(char *arg)
{

    util_trim(arg);

    if (util_assert_alpha(arg))
        exit(util_error("Could not parse alpha value '%s'.", arg));

    return arg;

}

static char *assert_digit(char *arg)
{

    util_trim(arg);

    if (util_assert_digit(arg))
        exit(util_error("Could not parse digit value '%s'.", arg));

    return arg;

}

static char *assert_xdigit(char *arg)
{

    util_trim(arg);

    if (util_assert_xdigit(arg))
        exit(util_error("Could not parse hex value '%s'.", arg));

    return arg;

}

static char *assert_print(char *arg)
{

    util_trim(arg);

    if (util_assert_print(arg))
        exit(util_error("Could not parse printable value '%s'.", arg));

    return arg;

}

static char *assert_list(char *arg)
{

    util_trim(arg);
    util_strip(arg);

    if (util_assert_printspace(arg))
        exit(util_error("Could not parse list:\n%s.", arg));

    return arg;

}

static int run_exec(char *id, unsigned int run, char *name, char *command)
{

    struct remote remote;
    int rc;

    if (remote_load(&remote, name))
        return error_remote_load(name);

    if (remote_init_optional(&remote))
        return error_remote_init(name);

    remote.run = run;

    if (remote_log_create(&remote, id))
        return util_error("Could not create log.");

    if (remote_log_open_stderr(&remote, id))
        return util_error("Could not open stderr log.");

    if (remote_log_open_stdout(&remote, id))
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

    if (remote_log_close(&remote))
        return util_error("Could not close log.");

    return rc;

}

static int run_send(char *id, unsigned int run, char *name, char *localpath, char *remotepath)
{

    struct remote remote;
    int rc;

    if (remote_load(&remote, name))
        return error_remote_load(name);

    if (remote_init_optional(&remote))
        return error_remote_init(name);

    remote.run = run;

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

        if (config_init())
            return error_init();

        if (remote_init_required(&remote, name, hostname))
            return error_remote_init(name);

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

    if (name)
    {

        unsigned int names = util_split(name);

        if (config_init())
            return error_init();

        if (key && value)
        {

            int keytype = remote_get_type(key);
            unsigned int i;

            if (keytype == -1)
                return util_error("Invalid key '%s'.", key);

            for (i = 0; (name = util_nextword(name, i, names)); i++)
            {

                struct remote remote;

                if (remote_load(&remote, name))
                    return error_remote_load(name);

                if (remote_set_value(&remote, keytype, value) == NULL)
                    return util_error("Could not run configure remote '%s'.", remote.name);

                if (remote_save(&remote))
                    return error_remote_save(name);

            }

            return EXIT_SUCCESS;

        }

        else if (key)
        {

            int keytype = remote_get_type(key);
            unsigned int i;

            if (keytype == -1)
                return util_error("Invalid key '%s'.", key);

            for (i = 0; (name = util_nextword(name, i, names)); i++)
            {

                struct remote remote;
                char *value;

                if (remote_load(&remote, name))
                    return error_remote_load(name);

                value = remote_get_value(&remote, keytype);

                printf("%s: %s\n", remote.name, value);

            }

            return EXIT_SUCCESS;

        }

        else
        {

            unsigned int i;

            for (i = 0; (name = util_nextword(name, i, names)); i++)
            {

                struct remote remote;

                if (remote_load(&remote, name))
                    return error_remote_load(name);

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

            return EXIT_SUCCESS;

        }

    }

    return error_missing();

}

static int parse_exec(int argc, char **argv)
{

    unsigned int parallel = 0;
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

            case 'p':
                parallel = 1;

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
                command = assert_print(arg);

                break;

            default:
                return error_toomany();

            }

        }

    }

    if (name && command)
    {

        unsigned int names = util_split(name);
        unsigned int total = 0;
        unsigned int complete = 0;
        unsigned int success = 0;
        char id[32];

        createid(id, 32);

        if (config_init())
            return error_init();

        if (event_begin(id))
            return util_error("Could not run event.");

        if (log_prepare(id))
            return util_error("Could not prepare log.");

        if (parallel)
        {

            unsigned int i;
            int status;

            for (i = 0; (name = util_nextword(name, i, names)); i++)
            {

                pid_t pid = fork();

                if (pid == 0)
                    return run_exec(id, i, name, command);

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

            unsigned int i;

            for (i = 0; (name = util_nextword(name, i, names)); i++)
            {

                total++;

                if (run_exec(id, i, name, command) == 0)
                    success++;

                complete++;

            }

        }

        if (event_end(total, complete, success))
            return util_error("Could not run event.");

        if (log_write_head(id, total, complete, success))
            return util_error("Could not log HEAD.");

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
            return util_error("Already initialized.");

        if (config_init())
            return error_init();

        if (config_get_path(path, BUFSIZ, "config"))
            return error_path();

        file = fopen(path, "w");

        if (file == NULL)
            return util_error("Could not create config file.");

        ini_write_section(file, "core");
        ini_write_string(file, "version", CONFIG_VERSION);
        fclose(file);

        if (config_get_path(path, BUFSIZ, CONFIG_HOOKS))
            return error_path();

        if (mkdir(path, 0775) < 0)
            return util_error("Could not create directory '%s'.", CONFIG_HOOKS);

        for (i = 0; hooks[i]; i++)
        {

            char buffer[BUFSIZ];
            unsigned int count;

            snprintf(buffer, BUFSIZ, "%s/%s.sample", CONFIG_HOOKS, hooks[i]);

            if (config_get_path(path, BUFSIZ, buffer))
                return error_path();

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

    return error_missing();

}

static int parse_list(int argc, char **argv)
{

    char *label = NULL;
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
                label = assert_print(arg);

                break;

            default:
                return error_toomany();

            }

        }

    }

    {

        struct dirent *entry;
        char path[BUFSIZ];
        DIR *dir;

        if (config_init())
            return error_init();

        if (config_get_path(path, BUFSIZ, CONFIG_REMOTES))
            return error_path();

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

    return error_missing();

}

static int parse_log(int argc, char **argv)
{

    unsigned int descriptor = 1;
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

            case 'e':
                descriptor = 2;

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
                id = assert_xdigit(arg);

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

        struct log_state state;
        struct log_entry entry;

        if (config_init())
            return error_init();

        log_open_head(&state);

        if (log_find(&state, &entry, id))
        {

            char buffer[BUFSIZ];
            unsigned int count;
            char path[BUFSIZ];
            int fd;

            switch (descriptor)
            {

            case 1:
                if (config_get_logsstdout(path, BUFSIZ, entry.id, run))
                    return error_path();

                break;

            case 2:
                if (config_get_logsstderr(path, BUFSIZ, entry.id, run))
                    return error_path();

                break;

            }

            fd = open(path, O_RDONLY, 0644);

            if (fd < 0)
                return util_error("Could not open '%s'.", path);

            while ((count = read(fd, buffer, BUFSIZ)))
                write(STDOUT_FILENO, buffer, count);

            close(fd);

        }

        return EXIT_SUCCESS;

    }

    else if (id)
    {

        struct log_state state;
        struct log_entry entry;

        if (config_init())
            return error_init();

        log_open_head(&state);

        if (log_find(&state, &entry, id))
            log_printentry(&entry);

        log_close_head(&state);

        return EXIT_SUCCESS;

    }

    else
    {

        struct log_state state;
        struct log_entry entry;

        if (config_init())
            return error_init();

        log_open_head(&state);

        while (log_readentryprev(&state, &entry))
            log_printentry(&entry);

        log_close_head(&state);

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
        struct remote remote;
        unsigned int i;

        if (config_init())
            return error_init();

        for (i = 0; (name = util_nextword(name, i, names)); i++)
        {

            if (remote_load(&remote, name))
                return error_remote_load(name);

            if (remote_erase(&remote))
                return util_error("Could not remove remote '%s'.", remote.name);

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
        char id[32];

        createid(id, 32);

        if (config_init())
            return error_init();

        for (i = 0; (name = util_nextword(name, i, names)); i++)
            run_send(id, i, name, localpath, remotepath);

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

        if (config_init())
            return error_init();

        if (remote_load(&remote, name))
            return error_remote_load(name);

        if (remote_init_optional(&remote))
            return error_remote_init(name);

        if (ssh_connect(&remote))
            return util_error("Could not connect to remote '%s'.", remote.name);

        if (ssh_shell(&remote))
            return util_error("Could not open shell on remote '%s'.", remote.name);

        if (ssh_disconnect(&remote))
            return util_error("Could not disconnect from remote '%s'.", remote.name);

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
        {"add", parse_add, " <name> <hostname>", 0},
        {"config", parse_config, " <namelist> [<key>] [<value>]", "List of keys:\n    name hostname port username password privatekey publickey label\n"},
        {"exec", parse_exec, " [-p] <namelist> <command>", "Args:\n    -p  Run in parallel\n"},
        {"init", parse_init, "", 0},
        {"list", parse_list, " [<label>]", 0},
        {"log", parse_log, " [-e] [<id>] [<run>]", "Args:\n    -e  Show stderr\n"},
        {"remove", parse_remove, " <namelist>", 0},
        {"send", parse_send, " <namelist> <localpath> <remotepath>", 0},
        {"shell", parse_shell, " <name>", 0},
        {"version", parse_version, "", 0},
        {0}
    };

    return assert_args(commands, argc - 1, argv + 1);

}

