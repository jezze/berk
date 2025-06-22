struct log
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

int log_open(struct log *log);
int log_close(struct log *log);
int log_prepare(struct log *log);
int log_read(struct log *log);
int log_readprev(struct log *log);
int log_find(struct log *log, char *id);
int log_printstd(struct log *log, unsigned int run, unsigned int descriptor);
int log_print(struct log *log);
int log_add(struct log *log);
int log_update(struct log *log);
void log_init(struct log *log, unsigned int total);
