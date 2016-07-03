#include <stdio.h>
#include <stdlib.h>
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
    int count;

    channel = libssh2_channel_open_session(session);

    if (channel == NULL)
        error(ERROR_PANIC, "Could not open SSH2 channel.");

    if (libssh2_channel_exec(channel, commandline) < 0)
        error(ERROR_PANIC, "Could not execute command over SSH2 channel.");

    do
    {

        char buffer[4096];
        unsigned int i;

        count = libssh2_channel_read(channel, buffer, 4096);

        if (count < 0)
            error(ERROR_PANIC, "Could not read from SSH2 channel.");

        for (i = 0; i < count; i++)
            fputc(buffer[i], stdout);

    }
    while (count > 0);

    libssh2_channel_close(channel);
    libssh2_channel_wait_closed(channel);

    exitcode = libssh2_channel_get_exit_status(channel);

    libssh2_channel_free(channel);

    return exitcode;

}

