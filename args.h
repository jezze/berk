#define ARGS_NONE 0
#define ARGS_COMMAND 1
#define ARGS_FLAG 2
#define ARGS_OPTION 3
#define ARGS_DONE 4
#define ARGS_ERROR 5

struct args
{

    unsigned int index;
    unsigned int position;
    unsigned int state;
    unsigned char flag;
    char *options;
    char *value;
    int argc;
    char **argv;

};

void args_setoptions(struct args *args, char *options);
void args_init(struct args *args, int argc, char **argv);
unsigned int args_next(struct args *args);
