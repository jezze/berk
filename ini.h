int ini_parse(char *filename, int (*handler)(void *user, char *section, char *key, char *value), void *user);
int ini_writesection(FILE *file, char *name);
int ini_writeint(FILE *file, char *key, int value);
int ini_writestring(FILE *file, char *key, char *value);
