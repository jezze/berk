int ini_parse(char *filename, int (*handler)(void *user, char *section, char *key, char *value), void *user);
int ini_write_section(FILE *file, char *name);
int ini_write_int(FILE *file, char *key, int value);
int ini_write_string(FILE *file, char *key, char *value);
