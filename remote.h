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
int remote_parse(int argc, char **argv);
