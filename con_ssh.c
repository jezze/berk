#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <libssh2.h>
#include "error.h"
#include "remote.h"
#include "con.h"

static LIBSSH2_SESSION *session;

int con_ssh_connect(struct remote *remote)
{

    if (con_connect(remote) < 0)
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

int con_ssh_disconnect(struct remote *remote)
{

    libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing");
    libssh2_session_free(session);
    libssh2_exit();

    if (con_disconnect(remote) < 0)
        error(ERROR_PANIC, "Could not disconnect from '%s'.", remote->name);

    return 0;

}

int con_ssh_exec(struct remote *remote, char *commandline)
{

    LIBSSH2_CHANNEL *channel;
    int exitcode;

    channel = libssh2_channel_open_session(session);

    if (channel == NULL)
        error(ERROR_PANIC, "Could not open SSH2 channel.");

    if (libssh2_channel_exec(channel, commandline) < 0)
        error(ERROR_PANIC, "Could not execute command over SSH2 channel.");

    do
    {

        char buffer[BUFSIZ];
        int count;

        count = libssh2_channel_read(channel, buffer, BUFSIZ);

        if (!count)
            break;

        count = remote_log(remote, buffer, count);

    } while (1);

    libssh2_channel_close(channel);
    libssh2_channel_wait_closed(channel);

    exitcode = libssh2_channel_get_exit_status(channel);

    libssh2_channel_free(channel);

    return exitcode;

}

int con_ssh_shell(struct remote *remote)
{

    LIBSSH2_CHANNEL *channel;
    int exitcode;
    struct pollfd pfds[2];

    memset(pfds, 0, sizeof(struct pollfd) * 2);

    pfds[0].fd = remote->sock;
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;
    pfds[1].fd = 0;
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
            count = write(1, buffer, count);

        }

        if (pfds[1].revents & POLLIN)
        {

            char buffer[BUFSIZ];
            int count;

            count = read(0, buffer, BUFSIZ);
            count = libssh2_channel_write(channel, buffer, count);

        }

    } while (!libssh2_channel_eof(channel));

    libssh2_channel_close(channel);
    libssh2_channel_wait_closed(channel);

    exitcode = libssh2_channel_get_exit_status(channel);

    libssh2_channel_free(channel);

    return exitcode;

}

