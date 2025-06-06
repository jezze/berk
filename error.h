int error(char *format, ...);
int error_init(void);
int error_remote_init(char *name);
int error_remote_load(char *name);
int error_remote_save(char *name);
int error_missing(void);
int error_toomany(void);
int error_flag_unrecognized(char *arg);
