void event_begin();
void event_end(unsigned int total, unsigned int complete, unsigned int success);
void event_start(struct remote *remote);
void event_stop(struct remote *remote, int status);
