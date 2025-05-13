int event_begin(char *id);
int event_end(unsigned int total, unsigned int complete, unsigned int success);
int event_start(struct remote *remote);
int event_stop(struct remote *remote, int status);
