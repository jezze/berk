#define ARGS_TYPE_NONE 0
#define ARGS_TYPE_COMMAND 1
#define ARGS_TYPE_FLAG 2

struct args_state
{

    unsigned int argp;
    unsigned int argi;
    unsigned int type;
    char flag;
    unsigned int hash;
    char *arg;

};

int args_next(struct args_state *state, int argc, char **argv);
unsigned int args_position(struct args_state *state);
void args_init(struct args_state *state);
