#define CONFIG_PROGNAME                 "berk"
#define CONFIG_VERSION                  "0.0.1"
#define CONFIG_ROOT                     ".berk"
#define CONFIG_LOGS                     "logs"
#define CONFIG_REMOTES                  "remotes"
#define CONFIG_HOOKS                    "hooks"

void config_init();
int config_getpath(char *path, unsigned int length, char *name);
int config_getremotepath(char *path, unsigned int length, char *filename);
int config_getlogpathbypid(char *path, unsigned int length, int pid);
int config_getlogpathbyname(char *path, unsigned int length, char *filename);
int config_gethookpath(char *path, unsigned int length, char *filename);
