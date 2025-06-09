#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <libssh2.h>
#include <netdb.h>
#include "util.h"
#include "log.h"
#include "run.h"
#include "remote.h"
#include "ssh.h"

int ssh_connect(struct remote *remote)
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

int ssh_disconnect(struct remote *remote)
{

    libssh2_session_disconnect(remote->session, "Normal Shutdown, Thank you for playing");
    libssh2_session_free(remote->session);
    libssh2_exit();
    close(remote->sock);

    return 0;

}

int ssh_exec(struct remote *remote, struct run *run, char *command)
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
                write(run->stdoutfd, buffer, count);

            count = libssh2_channel_read_stderr(remote->channel, buffer, BUFSIZ);

            if (count == LIBSSH2_ERROR_EAGAIN)
                continue;

            if (count > 0)
                write(run->stderrfd, buffer, count);

        }

    } while (!libssh2_channel_eof(remote->channel));

    libssh2_channel_close(remote->channel);
    libssh2_channel_wait_closed(remote->channel);

    rc = libssh2_channel_get_exit_status(remote->channel);

    libssh2_channel_free(remote->channel);

    return rc;

}

int ssh_send(struct remote *remote, char *localpath, char *remotepath)
{

    struct stat fileinfo;
    char buffer[BUFSIZ];
    size_t total;
    int fd;

    if (stat(localpath, &fileinfo))
        return -1;

    fd = open(localpath, O_RDONLY, 0644);

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

int ssh_shell(struct remote *remote, char *type)
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

