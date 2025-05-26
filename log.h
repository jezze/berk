struct log_entry
{

    char id[33];
    char datetime[25];
    unsigned int total;
    unsigned int complete;
    unsigned int success;
    int result;

};

struct log_state
{

    FILE *file;
    long size;
    long position;

};

int log_prepare(char *id);
int log_open_head(struct log_state *state);
void log_close_head(struct log_state *state);
long log_previous(struct log_state *state);
int log_readentry(struct log_state *state, struct log_entry *entry);
int log_find(struct log_state *state, struct log_entry *entry, char *id);
int log_printentry(struct log_entry *entry);
int log_write(int fd, char *buffer, unsigned int size);
int log_write_head(char *id, int total, int complete, int success);
