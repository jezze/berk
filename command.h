void command_add(char *name, char *hostname, char *username);
void command_config(struct remote *remote, char *key, char *value);
int command_exec(struct remote *remote, unsigned int pid, char *command);
void command_init();
void command_list();
void command_log(struct remote *remote, int pid);
void command_remove(struct remote *remote);
void command_shell(struct remote *remote);
void command_show(struct remote *remote);
