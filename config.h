#define CONFIG_PROGNAME                 "berk"
#define CONFIG_VERSION                  "0.0.1"
#define CONFIG_ROOT                     ".berk"
#define CONFIG_LOGS                     "logs"
#define CONFIG_REMOTES                  "remotes"
#define CONFIG_HOOKS                    "hooks"

int config_init();
int config_get_path(char *path, unsigned int length, char *name);
int config_get_remotepath(char *path, unsigned int length, char *filename);
int config_get_shortrun(char *path, unsigned int length, char *id);
int config_get_fullrun(char *path, unsigned int length, char *id);
int config_get_logdir(char *path, unsigned int length, char *id, int run);
int config_get_logvstderr(char *path, unsigned int length, char *id, int run);
int config_get_logsstderr(char *path, unsigned int length, char *id, char *run);
int config_get_logvstdout(char *path, unsigned int length, char *id, int run);
int config_get_logsstdout(char *path, unsigned int length, char *id, char *run);
int config_get_hookpath(char *path, unsigned int length, char *filename);
