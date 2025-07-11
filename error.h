int error(char *format, ...);
int error_init(void);
int error_remote_prepare(char *name);
int error_remote_load(char *name);
int error_remote_save(char *name);
int error_remote_connect(char *name);
int error_remote_disconnect(char *name);
int error_run_prepare(unsigned int index);
int error_run_update(unsigned int index, char *type);
int error_run_open(unsigned int index);
int error_run_close(unsigned int index);
int error_missing(void);
int error_toomany(void);
int error_flag_unrecognized(char *arg);
int error_arg_invalid(char *arg);
int error_arg_parse(char *arg, char *type);
