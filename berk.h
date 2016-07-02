#define BERK_ROOT                       ".berk"
#define BERK_CONFIG                     BERK_ROOT "/config"
#define BERK_REMOTES_BASE               BERK_ROOT "/remotes"
#define BERK_JOBS_BASE                  BERK_ROOT "/jobs"
#define BERK_NAME                       "berk"
#define BERK_VERSION                    "0.0.1"

struct berk_command
{

    char *name;
    int (*parse)(int argc, char **argv);
    char *description;

};

int berk_error_command(char *name, struct berk_command *cmds);
int berk_error_extra();
int berk_error_invalid(char *arg);
int berk_error_missing();
int berk_error(char *msg);
void berk_panic(char *msg);
