struct run
{

    unsigned int index;
    int stderrfd;
    int stdoutfd;

};

int run_open(struct run *run, struct log_entry *entry);
int run_close(struct run *run);
void run_init(struct run *run, unsigned int index);
