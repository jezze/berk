#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <termios.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <libssh2.h>
#include <netdb.h>
#include "error.h"
#include "remote.h"
#include "ssh.h"

static LIBSSH2_SESSION *session;
static struct termios old;
static struct termios new;

static int opensocket(struct remote *remote)
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

    return remote->sock;

}

static int closesocket(struct remote *remote)
{

    return close(remote->sock);

}

int ssh_connect(struct remote *remote)
{

    if (opensocket(remote) < 0)
        error(ERROR_PANIC, "Could not connect to '%s'.", remote->name);

    if (libssh2_init(0) < 0)
        error(ERROR_PANIC, "Could not initialize SSH2.");

    session = libssh2_session_init();

    if (session == NULL)
        error(ERROR_PANIC, "Could not initialize SSH2 session.");

    if (libssh2_session_handshake(session, remote->sock) < 0)
        error(ERROR_PANIC, "Could not handshake SSH2 session.");

    if (libssh2_userauth_publickey_fromfile(session, remote->username, remote->publickey, remote->privatekey, 0) < 0)
        error(ERROR_PANIC, "Could not authorize user '%s' with keyfiles '%s' and '%s'.", remote->username, remote->privatekey, remote->publickey);

    return 0;

}

int ssh_disconnect(struct remote *remote)
{

    libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing");
    libssh2_session_free(session);
    libssh2_exit();

    if (closesocket(remote) < 0)
        error(ERROR_PANIC, "Could not disconnect from '%s'.", remote->name);

    return 0;

}

int ssh_exec(struct remote *remote, char *command)
{

    LIBSSH2_CHANNEL *channel;
    struct pollfd pfds[1];
    int exitcode;

    pfds[0].fd = remote->sock;
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;

    channel = libssh2_channel_open_session(session);

    if (channel == NULL)
        error(ERROR_PANIC, "Could not open SSH2 channel.");

    if (libssh2_channel_exec(channel, command) < 0)
        error(ERROR_PANIC, "Could not execute command over SSH2 channel.");

    libssh2_channel_set_blocking(channel, 0);

    do
    {

        int status = poll(pfds, 1, -1);

        if (status == -1)
            break;

        if (pfds[0].revents & POLLIN)
        {

            char buffer[BUFSIZ];
            int count;

            count = libssh2_channel_read(channel, buffer, BUFSIZ);
            count = remote_log(remote, buffer, count);

        }

    } while (!libssh2_channel_eof(channel));

    libssh2_channel_close(channel);
    libssh2_channel_wait_closed(channel);

    exitcode = libssh2_channel_get_exit_status(channel);

    libssh2_channel_free(channel);

    return exitcode;

}

int ssh_shell(struct remote *remote)
{

    LIBSSH2_CHANNEL *channel;
    struct pollfd pfds[2];
    int exitcode;

    pfds[0].fd = remote->sock;
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;
    pfds[1].fd = STDIN_FILENO;
    pfds[1].events = POLLIN;
    pfds[1].revents = 0;

    channel = libssh2_channel_open_session(session);

    if (channel == NULL)
        error(ERROR_PANIC, "Could not open SSH2 channel.");

    if (libssh2_channel_request_pty(channel, "vt102") < 0)
        error(ERROR_PANIC, "Could not start pty over SSH2 channel.");

    if (libssh2_channel_shell(channel) < 0)
        error(ERROR_PANIC, "Could not start shell over SSH2 channel.");

    libssh2_channel_set_blocking(channel, 0);
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

            count = libssh2_channel_read(channel, buffer, BUFSIZ);
            count = write(STDOUT_FILENO, buffer, count);

        }

        if (pfds[1].revents & POLLIN)
        {

            char buffer[BUFSIZ];
            int count;

            count = read(STDIN_FILENO, buffer, BUFSIZ);
            count = libssh2_channel_write(channel, buffer, count);

        }

    } while (!libssh2_channel_eof(channel));

    tcsetattr(0, TCSANOW, &old);
    libssh2_channel_close(channel);
    libssh2_channel_wait_closed(channel);

    exitcode = libssh2_channel_get_exit_status(channel);

    libssh2_channel_free(channel);

    return exitcode;

}

