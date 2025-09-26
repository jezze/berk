#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <pwd.h>
#include <termios.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <libssh2.h>
#include <netdb.h>
#include "config.h"
#include "util.h"
#include "ini.h"
#include "remote.h"

void *remote_get_value(struct remote *remote, unsigned int hash)
{

    switch (hash)
    {

    case REMOTE_NAME:
        return remote->name;

    case REMOTE_TYPE:
        return remote->type;

    case REMOTE_HOSTNAME:
        return remote->hostname;

    case REMOTE_PORT:
        return remote->port;

    case REMOTE_USERNAME:
        return remote->username;

    case REMOTE_PASSWORD:
        return remote->password;

    case REMOTE_PRIVATEKEY:
        return remote->privatekey;

    case REMOTE_PUBLICKEY:
        return remote->publickey;

    case REMOTE_TAGS:
        return remote->tags;

    }

    return NULL;

}

unsigned int remote_set_value(struct remote *remote, unsigned int hash, char *value)
{

    char *v = (value != NULL) ? strdup(value) : NULL;

    switch (hash)
    {

    case REMOTE_NAME:
        remote->name = v;

        break;

    case REMOTE_TYPE:
        remote->type = v;
        remote->typehash = util_hash(v);

        break;

    case REMOTE_HOSTNAME:
        remote->hostname = v;

        break;

    case REMOTE_PORT:
        remote->port = v;

        break;

    case REMOTE_USERNAME:
        remote->username = v;

        break;

    case REMOTE_PASSWORD:
        remote->password = v;

        break;

    case REMOTE_PRIVATEKEY:
        remote->privatekey = v;

        break;

    case REMOTE_PUBLICKEY:
        remote->publickey = v;

        break;

    case REMOTE_TAGS:
        remote->tags = v;

        break;

    default:
        hash = 0;

        break;

    }

    return hash;

}

static int loadcallback(void *user, char *section, char *key, char *value)
{

    struct remote *remote = user;
    unsigned int hash = util_hash(key);

    if (strcmp(section, "remote"))
        return 0;

    if (strlen(value))
        remote_set_value(remote, hash, value);

    return 0;

}

int remote_load(struct remote *remote)
{

    char path[BUFSIZ];

    config_get_subpath(path, BUFSIZ, CONFIG_REMOTES, remote->name);

    return ini_parse(path, loadcallback, remote);

}

int remote_save(struct remote *remote)
{

    char path[BUFSIZ];
    int fd;

    config_get_path(path, BUFSIZ, CONFIG_REMOTES);

    if (util_mkdir(path) < 0)
        return -1;

    config_get_subpath(path, BUFSIZ, CONFIG_REMOTES, remote->name);

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (fd < 0)
        return -1;

    ini_write_section(fd, "remote");

    if (remote->name && strlen(remote->name))
        ini_write_string(fd, "name", remote->name);

    if (remote->type && strlen(remote->type))
        ini_write_string(fd, "type", remote->type);

    if (remote->hostname && strlen(remote->hostname))
        ini_write_string(fd, "hostname", remote->hostname);

    if (remote->port && strlen(remote->port))
        ini_write_string(fd, "port", remote->port);

    if (remote->username && strlen(remote->username))
        ini_write_string(fd, "username", remote->username);

    if (remote->password && strlen(remote->password))
        ini_write_string(fd, "password", remote->password);

    if (remote->privatekey && strlen(remote->privatekey))
        ini_write_string(fd, "privatekey", remote->privatekey);

    if (remote->publickey && strlen(remote->publickey))
        ini_write_string(fd, "publickey", remote->publickey);

    if (remote->tags && strlen(remote->tags))
        ini_write_string(fd, "tags", remote->tags);

    close(fd);

    return 0;

}

int remote_remove(struct remote *remote)
{

    char path[BUFSIZ];

    config_get_subpath(path, BUFSIZ, CONFIG_REMOTES, remote->name);

    return util_unlink(path);

}

static int remote_prepare_local(struct remote *remote)
{

    return 0;

}

static int remote_prepare_ssh(struct remote *remote)
{

    char buffer[BUFSIZ];
    char keybuffer[BUFSIZ];
    struct passwd passwd, *current;

    if (!remote->hostname)    
        remote_set_value(remote, REMOTE_HOSTNAME, "localhost");

    if (!remote->port)
        remote_set_value(remote, REMOTE_PORT, "22");

    if (!remote->username)    
        remote_set_value(remote, REMOTE_USERNAME, getenv("USER"));

    if (getpwnam_r(remote->username, &passwd, buffer, BUFSIZ, &current))
        return -1;

    if (!remote->privatekey)
    {

        snprintf(keybuffer, BUFSIZ, "%s/.ssh/%s", passwd.pw_dir, "id_rsa");
        remote_set_value(remote, REMOTE_PRIVATEKEY, keybuffer);

    }

    if (!remote->publickey)
    {

        snprintf(keybuffer, BUFSIZ, "%s/.ssh/%s", passwd.pw_dir, "id_rsa.pub");
        remote_set_value(remote, REMOTE_PUBLICKEY, keybuffer);

    }

    return 0;

}

int remote_prepare(struct remote *remote)
{

    switch (remote->typehash)
    {

    case REMOTE_TYPE_LOCAL:
        return remote_prepare_local(remote);

    case REMOTE_TYPE_SSH:
        return remote_prepare_ssh(remote);

    }

    return -1;

}

static int remote_connect_local(struct remote *remote)
{

    return 0;

}

static int remote_connect_ssh(struct remote *remote)
{

    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof (struct addrinfo));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(remote->hostname, remote->port, &hints, &servinfo) != 0)
        return -1;

    remote->sock = socket(servinfo->ai_family, servinfo->ai_socktype, 0);

    if (remote->sock < 0)
        return -1;

    if (connect(remote->sock, servinfo->ai_addr, servinfo->ai_addrlen) < 0)
        return -1;

    if (libssh2_init(0) < 0)
        return -1;

    remote->session = libssh2_session_init();

    if (remote->session == NULL)
        return -1;

    if (libssh2_session_handshake(remote->session, remote->sock) < 0)
        return -1;

    if (remote->password)
    {

        if (libssh2_userauth_password(remote->session, remote->username, remote->password) < 0)
            return -1;

    }

    else
    {

        if (libssh2_userauth_publickey_fromfile(remote->session, remote->username, remote->publickey, remote->privatekey, 0) < 0)
            return -1;

    }

    return 0;

}

int remote_connect(struct remote *remote)
{

    switch (remote->typehash)
    {

    case REMOTE_TYPE_LOCAL:
        return remote_connect_local(remote);

    case REMOTE_TYPE_SSH:
        return remote_connect_ssh(remote);

    }

    return -1;

}

static int remote_disconnect_local(struct remote *remote)
{

    return 0;

}

static int remote_disconnect_ssh(struct remote *remote)
{

    libssh2_session_disconnect(remote->session, "Normal Shutdown, Thank you for playing");
    libssh2_session_free(remote->session);
    libssh2_exit();
    close(remote->sock);

    return 0;

}

int remote_disconnect(struct remote *remote)
{

    switch (remote->typehash)
    {

    case REMOTE_TYPE_LOCAL:
        return remote_disconnect_local(remote);

    case REMOTE_TYPE_SSH:
        return remote_disconnect_ssh(remote);

    }

    return -1;

}

static int remote_exec_local(struct remote *remote, char *command, int stdoutfd, int stderrfd)
{

    int oldstderrfd = dup(STDERR_FILENO);
    int oldstdoutfd = dup(STDOUT_FILENO);
    int rc;

    dup2(stderrfd, STDERR_FILENO);
    dup2(stdoutfd, STDOUT_FILENO);

    rc = system(command);

    dup2(oldstderrfd, STDERR_FILENO);
    dup2(oldstdoutfd, STDOUT_FILENO);

    return rc;

}

static int remote_exec_ssh(struct remote *remote, char *command, int stdoutfd, int stderrfd)
{

    struct pollfd pfds[1];
    int rc;

    pfds[0].fd = remote->sock;
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;

    remote->channel = libssh2_channel_open_session(remote->session);

    if (remote->channel == NULL)
        return -1;

    if (libssh2_channel_exec(remote->channel, command) < 0)
        return -1;

    libssh2_channel_set_blocking(remote->channel, 0);

    do
    {

        int status = poll(pfds, 1, -1);

        if (status == -1)
            break;

        if (pfds[0].revents & POLLIN)
        {

            char buffer[BUFSIZ];
            int count;

            count = libssh2_channel_read(remote->channel, buffer, BUFSIZ);

            if (count == LIBSSH2_ERROR_EAGAIN)
                continue;

            if (count > 0)
                write(stdoutfd, buffer, count);

            count = libssh2_channel_read_stderr(remote->channel, buffer, BUFSIZ);

            if (count == LIBSSH2_ERROR_EAGAIN)
                continue;

            if (count > 0)
                write(stderrfd, buffer, count);

        }

    } while (!libssh2_channel_eof(remote->channel));

    libssh2_channel_close(remote->channel);
    libssh2_channel_wait_closed(remote->channel);

    rc = libssh2_channel_get_exit_status(remote->channel);

    libssh2_channel_free(remote->channel);

    return rc;

}

int remote_exec(struct remote *remote, char *command, int stdoutfd, int stderrfd)
{

    switch (remote->typehash)
    {

    case REMOTE_TYPE_LOCAL:
        return remote_exec_local(remote, command, stdoutfd, stderrfd);

    case REMOTE_TYPE_SSH:
        return remote_exec_ssh(remote, command, stdoutfd, stderrfd);

    }

    return -1;

}

static int remote_send_local(struct remote *remote, char *localpath, char *remotepath)
{

    struct stat fileinfo;
    char buffer[BUFSIZ];
    size_t total;
    int fdlocal;
    int fdremote;

    if (stat(localpath, &fileinfo))
        return -1;

    fdlocal = open(localpath, O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (fdlocal < 0)
        return -1;

    fdremote = open(remotepath, O_WRONLY | O_CREAT | O_SYNC, fileinfo.st_mode);

    if (fdremote < 0)
        return -1;

    while ((total = read(fdlocal, buffer, BUFSIZ)))
    {

        char *current = buffer;

        if (total < 0)
            break;

        do
        {

            int count = write(fdremote, current, total);

            if (count < 0)
                break;

            current += count;
            total -= count;

        } while (total);
 
    };

    close(fdlocal);
    close(fdremote);

    return 0;

}

static int remote_send_ssh(struct remote *remote, char *localpath, char *remotepath)
{

    struct stat fileinfo;
    char buffer[BUFSIZ];
    size_t total;
    int fd;

    if (stat(localpath, &fileinfo))
        return -1;

    fd = open(localpath, O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (fd < 0)
        return -1;

    remote->channel = libssh2_scp_send(remote->session, remotepath, fileinfo.st_mode & 0777, fileinfo.st_size);

    if (remote->channel == NULL)
        return -1;

    while ((total = read(fd, buffer, BUFSIZ)))
    {

        char *current = buffer;

        if (total < 0)
            break;

        do
        {

            int count = libssh2_channel_write(remote->channel, current, total);

            if (count < 0)
                break;

            current += count;
            total -= count;

        } while (total);
 
    };

    libssh2_channel_send_eof(remote->channel);
    libssh2_channel_wait_eof(remote->channel);
    libssh2_channel_wait_closed(remote->channel);
    libssh2_channel_free(remote->channel);
    close(fd);

    return 0;

}

int remote_send(struct remote *remote, char *localpath, char *remotepath)
{

    switch (remote->typehash)
    {

    case REMOTE_TYPE_LOCAL:
        return remote_send_local(remote, localpath, remotepath);

    case REMOTE_TYPE_SSH:
        return remote_send_ssh(remote, localpath, remotepath);

    }

    return -1;

}

static int remote_shell_ssh(struct remote *remote, char *type)
{

    struct pollfd pfds[2];
    struct termios old;
    struct termios new;
    int rc;

    pfds[0].fd = remote->sock;
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;
    pfds[1].fd = STDIN_FILENO;
    pfds[1].events = POLLIN;
    pfds[1].revents = 0;

    remote->channel = libssh2_channel_open_session(remote->session);

    if (remote->channel == NULL)
        return -1;

    if (libssh2_channel_request_pty(remote->channel, type) < 0)
        return -1;

    if (libssh2_channel_shell(remote->channel) < 0)
        return -1;

    libssh2_channel_set_blocking(remote->channel, 0);
    cfmakeraw(&new);
    tcgetattr(0, &old);
    tcsetattr(0, TCSANOW, &new);

    do
    {

        int status = poll(pfds, 2, -1);

        if (status == -1)
            break;

        if (pfds[0].revents & POLLIN)
        {

            char buffer[BUFSIZ];
            int count;

            count = libssh2_channel_read(remote->channel, buffer, BUFSIZ);

            if (count == LIBSSH2_ERROR_EAGAIN)
                continue;

            if (count > 0)
                write(STDOUT_FILENO, buffer, count);

        }

        if (pfds[1].revents & POLLIN)
        {

            char buffer[BUFSIZ];
            int count;

            count = read(STDIN_FILENO, buffer, BUFSIZ);
            count = libssh2_channel_write(remote->channel, buffer, count);

            if (count == LIBSSH2_ERROR_EAGAIN)
                continue;

        }

    } while (!libssh2_channel_eof(remote->channel));

    tcsetattr(0, TCSANOW, &old);
    libssh2_channel_close(remote->channel);
    libssh2_channel_wait_closed(remote->channel);

    rc = libssh2_channel_get_exit_status(remote->channel);

    libssh2_channel_free(remote->channel);

    return rc;

}

int remote_shell(struct remote *remote, char *type)
{

    switch (remote->typehash)
    {

    case REMOTE_TYPE_SSH:
        return remote_shell_ssh(remote, type);

    }

    return -1;

}

void remote_init(struct remote *remote, char *name)
{

    memset(remote, 0, sizeof (struct remote));
    remote_set_value(remote, REMOTE_NAME, name);
    remote_set_value(remote, REMOTE_TYPE, "ssh");

}

