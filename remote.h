#define REMOTE_NAME 0x7c9b0c46
#define REMOTE_TYPE 0x7c9ebd07
#define REMOTE_HOSTNAME 0xeba474a4
#define REMOTE_PORT 0x7c9c614a
#define REMOTE_USERNAME 0xfe18cd45
#define REMOTE_PASSWORD 0x17f6dc38
#define REMOTE_PRIVATEKEY 0x26c0b7c9
#define REMOTE_PUBLICKEY 0x5e00d6cd
#define REMOTE_TAGS 0x7c9e55d4

struct remote
{

    char *name;
    char *type;
    char *hostname;
    char *port;
    char *username;
    char *password;
    char *publickey;
    char *privatekey;
    char *tags;
    int sock;
    LIBSSH2_SESSION *session;
    LIBSSH2_CHANNEL *channel;

};

void *remote_get_value(struct remote *remote, unsigned int hash);
unsigned int remote_set_value(struct remote *remote, unsigned int hash, char *value);
int remote_load(struct remote *remote);
int remote_save(struct remote *remote);
int remote_erase(struct remote *remote);
int remote_prepare(struct remote *remote);
int remote_connect(struct remote *remote);
int remote_disconnect(struct remote *remote);
int remote_exec(struct remote *remote, struct run *run, char *command);
int remote_send(struct remote *remote, char *localpath, char *remotepath);
void remote_init(struct remote *remote, char *name);

