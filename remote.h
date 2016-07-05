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

int remote_load(struct remote *remote, char *name);
int remote_save(struct remote *remote);
int remote_erase(struct remote *remote);
int remote_log_open(struct remote *remote);
void remote_log_close(struct remote *remote);
int remote_log(struct remote *remote, char *buffer, unsigned int size);
int remote_log_print(struct remote *remote);
