struct log_entry
{

    char id[33];
    char datetime[25];
    unsigned int total;
    unsigned int complete;
    unsigned int passed;
    unsigned int failed;
    unsigned int offset;

};

struct log_state
{

    int fd;
    long size;
    long position;

};

int log_state_open(struct log_state *state);
int log_state_close(struct log_state *state);
int log_entry_prepare(struct log_entry *entry);
int log_entry_read(struct log_entry *entry, struct log_state *state);
int log_entry_readprev(struct log_entry *entry, struct log_state *state);
int log_entry_find(struct log_entry *entry, struct log_state *state, char *id);
int log_entry_printstd(struct log_entry *entry, unsigned int run, unsigned int descriptor);
int log_entry_print(struct log_entry *entry);
int log_entry_add(struct log_entry *entry);
int log_entry_update(struct log_entry *entry);
void log_entry_init(struct log_entry *entry, unsigned int total);
