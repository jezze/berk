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
    int gid;
    int pid;

};

void *remote_getvalue(struct remote *remote, char *key);
int remote_load(struct remote *remote, char *name);
int remote_save(struct remote *remote);
int remote_erase(struct remote *remote);
int remote_init(struct remote *remote, char *name, char *hostname, char *username);
int remote_config(struct remote *remote, char *key, char *value);
int remote_openlog(struct remote *remote);
void remote_closelog(struct remote *remote);
int remote_log(struct remote *remote, char *buffer, unsigned int size);
