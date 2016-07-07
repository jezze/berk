enum
{

    REMOTE_NAME,
    REMOTE_HOSTNAME,
    REMOTE_PORT,
    REMOTE_USERNAME,
    REMOTE_PRIVATEKEY,
    REMOTE_PUBLICKEY,
    REMOTE_LABEL

};

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

};

int remote_gettype(char *key);
int remote_load(struct remote *remote, char *name);
int remote_save(struct remote *remote);
int remote_erase(struct remote *remote);
int remote_add(char *name, char *hostname, char *username);
int remote_config(struct remote *remote, char *key, char *value);
int remote_log_open(struct remote *remote);
void remote_log_close(struct remote *remote);
int remote_log(struct remote *remote, char *buffer, unsigned int size);
int remote_log_print(struct remote *remote);
