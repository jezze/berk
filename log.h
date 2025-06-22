struct log_entry
{

    char id[33];
    char datetime[25];
    unsigned int total;
    unsigned int complete;
    unsigned int aborted;
    unsigned int passed;
    unsigned int failed;
    int fd;
    long size;
    long position;

};

int log_entry_open(struct log_entry *entry);
int log_entry_close(struct log_entry *entry);
int log_entry_prepare(struct log_entry *entry);
int log_entry_read(struct log_entry *entry);
int log_entry_readprev(struct log_entry *entry);
int log_entry_find(struct log_entry *entry, char *id);
int log_entry_printstd(struct log_entry *entry, unsigned int run, unsigned int descriptor);
int log_entry_print(struct log_entry *entry);
int log_entry_add(struct log_entry *entry);
int log_entry_update(struct log_entry *entry);
void log_entry_init(struct log_entry *entry, unsigned int total);
