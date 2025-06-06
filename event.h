int event_begin(struct log_entry *entry);
int event_end(struct log_entry *entry);
int event_start(struct remote *remote, struct run *run);
int event_stop(struct remote *remote, struct run *run);
int event_send(struct remote *remote);
