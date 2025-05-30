int event_begin(struct log_entry *entry);
int event_end(struct log_entry *entry);
int event_start(struct remote *remote);
int event_stop(struct remote *remote, int status);
