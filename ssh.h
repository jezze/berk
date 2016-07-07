int ssh_connect(struct remote *remote);
int ssh_disconnect(struct remote *remote);
int ssh_exec(struct remote *remote, char *command);
int ssh_shell(struct remote *remote);
