int ssh_connect(struct remote *remote);
int ssh_disconnect(struct remote *remote);
int ssh_exec(struct remote *remote, struct run *run, char *command);
int ssh_send(struct remote *remote, char *localpath, char *remotepath);
int ssh_shell(struct remote *remote, char *type);
