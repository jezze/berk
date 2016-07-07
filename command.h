void command_config(struct remote *remote, char *key, char *value);
int command_exec(struct remote *remote, char *command);
void command_init();
void command_list(char *label);
void command_shell(struct remote *remote);
void command_show(struct remote *remote, char *key);
