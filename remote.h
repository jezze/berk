struct remote
{

    char *name;
    char *hostname;
    char *port;
    char *username;
    char *publickey;
    char *privatekey;
    char *label;
    int sock;
    int logfd;
    int pid;
    LIBSSH2_SESSION *session;
    LIBSSH2_CHANNEL *channel;

};

int remote_gettype(char *key);
void *remote_getvalue(struct remote *remote, int type);
void *remote_setvalue(struct remote *remote, int type, char *value);
int remote_load(struct remote *remote, char *name);
int remote_save(struct remote *remote);
int remote_erase(struct remote *remote);
int remote_initrequired(struct remote *remote, char *name, char *hostname);
int remote_initoptional(struct remote *remote);
int remote_logprepare(int gid);
int remote_loghead(int gid, int total, int complete, int success);
int remote_openlog(struct remote *remote, int gid);
void remote_closelog(struct remote *remote);
int remote_log(struct remote *remote, char *buffer, unsigned int size);
