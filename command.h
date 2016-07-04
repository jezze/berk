void command_config(struct remote *remote, char *key, char *value);
void command_copy(struct remote *remote, char *name);
void command_create(char *name, char *hostname, char *username);
void command_exec(struct remote *remote, unsigned int num, char *command);
void command_init();
void command_list();
void command_remove(struct remote *remote);
void command_shell(struct remote *remote);
void command_show(struct remote *remote);
