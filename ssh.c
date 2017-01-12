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
#include "util.h"
#include "remote.h"
#include "ssh.h"

int ssh_connect(struct remote *remote)
{

    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof (struct addrinfo));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(remote->hostname, remote->port, &hints, &servinfo) != 0)
        return util_error("Could not get address info.");

    remote->sock = socket(servinfo->ai_family, servinfo->ai_socktype, 0);

    if (remote->sock < 0)
        return util_error("Could not create socket.");

    if (connect(remote->sock, servinfo->ai_addr, servinfo->ai_addrlen) < 0)
        return util_error("Could not connect to socket.");

    if (libssh2_init(0) < 0)
        return util_error("Could not initialize SSH2.");

    remote->session = libssh2_session_init();

    if (remote->session == NULL)
        return util_error("Could not initialize session.");

    if (libssh2_session_handshake(remote->session, remote->sock) < 0)
        return util_error("Could not handshake session.");

    if (libssh2_userauth_publickey_fromfile(remote->session, remote->username, remote->publickey, remote->privatekey, 0) < 0)
        return util_error("Could not authorize user '%s' with keyfiles '%s' and '%s'.", remote->username, remote->privatekey, remote->publickey);

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

int ssh_exec(struct remote *remote, char *command)
{

    struct pollfd pfds[1];
    int exitcode;

    pfds[0].fd = remote->sock;
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;

    remote->channel = libssh2_channel_open_session(remote->session);

    if (remote->channel == NULL)
        return util_error("Could not open channel.");

    if (libssh2_channel_exec(remote->channel, command) < 0)
        return util_error("Could not execute command over channel.");

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

            if (count == 0)
                break;

            if (count > 0)
                remote_log(remote, buffer, count);

        }

    } while (!libssh2_channel_eof(remote->channel));

    libssh2_channel_close(remote->channel);
    libssh2_channel_wait_closed(remote->channel);

    exitcode = libssh2_channel_get_exit_status(remote->channel);

    libssh2_channel_free(remote->channel);

    return exitcode;

}

int ssh_send(struct remote *remote, char *filepath)
{

    struct stat fileinfo;
    FILE *file;
    char mem[BUFSIZ];
    char *ptr;

    file = fopen(filepath, "rb");

    if (file == NULL)
        return util_error("Could not open file.");

    if (stat(filepath, &fileinfo))
        return util_error("Could not stat file.");

    remote->channel = libssh2_scp_send(remote->session, "test.txt", fileinfo.st_mode & 0777, fileinfo.st_size);

    if (remote->channel == NULL)
        return util_error("Could not open channel.");

    do
    {

        size_t nread = fread(mem, 1, BUFSIZ, file);
        int rc;

        if (nread <= 0)
        {

            break;

        }

        ptr = mem;
 
        do
        {

            rc = libssh2_channel_write(remote->channel, ptr, nread);

            if (rc < 0)
            {

                fprintf(stderr, "ERROR %d\n", rc);

                break;

            }

            else
            {

                ptr += rc;
                nread -= rc;

            }

        } while (nread);
 
    } while (1);

    libssh2_channel_send_eof(remote->channel);
    libssh2_channel_wait_eof(remote->channel);
    libssh2_channel_wait_closed(remote->channel);
    libssh2_channel_free(remote->channel);
    fclose(file);

    return 0;

}

int ssh_shell(struct remote *remote)
{

    struct pollfd pfds[2];
    struct termios old;
    struct termios new;
    int exitcode;

    pfds[0].fd = remote->sock;
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;
    pfds[1].fd = STDIN_FILENO;
    pfds[1].events = POLLIN;
    pfds[1].revents = 0;

    remote->channel = libssh2_channel_open_session(remote->session);

    if (remote->channel == NULL)
        return util_error("Could not open channel.");

    if (libssh2_channel_request_pty(remote->channel, "vt102") < 0)
        return util_error("Could not start pty over channel.");

    if (libssh2_channel_shell(remote->channel) < 0)
        return util_error("Could not start shell over channel.");

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

            if (count == 0)
                break;

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

    exitcode = libssh2_channel_get_exit_status(remote->channel);

    libssh2_channel_free(remote->channel);

    return exitcode;

}

