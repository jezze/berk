struct remote
{

    char name[32];
    char hostname[64];
    unsigned int port;
    char username[64];
    char publickey[64];
    char privatekey[64];
    int sock;

};

int remote_load(char *remote_name, struct remote *remote);
int remote_parse(int argc, char **argv);
