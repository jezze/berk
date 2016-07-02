int ini_parse(const char *filename, int (*handler)(void *user, const char *section, const char *name, const char *value), void *user);
int ini_writesection(FILE *file, char *name);
int ini_writeint(FILE *file, char *key, int value);
int ini_writestring(FILE *file, char *key, char *value);
