struct remote
{

    char *name;
    char *hostname;
    unsigned int port;
    char *username;
    char *publickey;
    char *privatekey;
    int sock;
    int logfd;
    int pid;

};

int remote_load(struct remote *remote, char *name);
int remote_save(struct remote *remote);
int remote_erase(struct remote *remote);
int remote_log_open(struct remote *remote);
void remote_log_close(struct remote *remote);
int remote_log(struct remote *remote, char *buffer, unsigned int size);
