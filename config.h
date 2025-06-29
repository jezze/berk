#define CONFIG_PROGNAME                 "berk"
#define CONFIG_VERSION                  "0.0.1"
#define CONFIG_ROOT                     ".berk"
#define CONFIG_PATH                     "config"
#define CONFIG_LOGS                     "logs"
#define CONFIG_REMOTES                  "remotes"
#define CONFIG_HOOKS                    "hooks"

struct config_core
{

    char *version;

};

char *config_init();
int config_load(struct config_core *core);
int config_save(struct config_core *core);
int config_get_path(char *path, unsigned int length, char *name);
int config_get_subpath(char *path, unsigned int length, char *dir, char *name);
int config_get_rundirshort(char *path, unsigned int length, char *id);
int config_get_rundirfull(char *path, unsigned int length, char *id);
int config_get_rundir(char *path, unsigned int length, char *id, int run);
int config_get_runpath(char *path, unsigned int length, char *id, int run, char *name);
