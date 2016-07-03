struct remote
{

    char *name;
    char *hostname;
    unsigned int port;
    char *username;
    char *publickey;
    char *privatekey;
    int sock;

};

int remote_load(char *filename, struct remote *remote);
int remote_save(char *filename, struct remote *remote);
void remote_copy(struct remote *remote, char *name);
void remote_create(char *name, char *hostname, char *username);
void remote_list();
void remote_remove(struct remote *remote);
void remote_show(struct remote *remote);
