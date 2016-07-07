void con_setraw();
void con_unsetraw();
int con_connect(struct remote *remote);
int con_disconnect(struct remote *remote);
int con_exec(struct remote *remote, char *command);
int con_shell(struct remote *remote);
