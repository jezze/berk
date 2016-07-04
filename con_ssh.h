int con_ssh_connect(struct remote *remote);
int con_ssh_disconnect(struct remote *remote);
int con_ssh_exec(struct remote *remote, char *commandline);
int con_ssh_shell(struct remote *remote);
