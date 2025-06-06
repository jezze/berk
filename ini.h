int ini_parse(char *filename, int (*handler)(void *user, char *section, char *key, char *value), void *user);
int ini_write_section(int fd, char *name);
int ini_write_int(int fd, char *key, int value);
int ini_write_string(int fd, char *key, char *value);
