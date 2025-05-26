struct remote
{

    char *name;
    char *hostname;
    char *port;
    char *username;
    char *password;
    char *publickey;
    char *privatekey;
    char *label;
    int sock;
    int stderrfd;
    int stdoutfd;
    int run;
    LIBSSH2_SESSION *session;
    LIBSSH2_CHANNEL *channel;

};

int remote_get_type(char *key);
void *remote_get_value(struct remote *remote, int type);
void *remote_set_value(struct remote *remote, int type, char *value);
int remote_load(struct remote *remote, char *name);
int remote_save(struct remote *remote);
int remote_erase(struct remote *remote);
int remote_init_required(struct remote *remote, char *name, char *hostname);
int remote_init_optional(struct remote *remote);
int remote_log_create(struct remote *remote, char *id);
int remote_log_open_stderr(struct remote *remote, char *id);
int remote_log_open_stdout(struct remote *remote, char *id);
int remote_log_close(struct remote *remote);
