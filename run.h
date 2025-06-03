enum run_status
{

    RUN_STATUS_PENDING,
    RUN_STATUS_PASSED,
    RUN_STATUS_FAILED

};

struct run
{

    unsigned int index;
    int stderrfd;
    int stdoutfd;

};

int run_prepare(struct run *run, struct log_entry *entry);
int run_update_remote(struct run *run, struct log_entry *entry, char *remote);
int run_update_status(struct run *run, struct log_entry *entry, int status);
int run_update_pid(struct run *run, struct log_entry *entry, unsigned int pid);
int run_open(struct run *run, struct log_entry *entry);
int run_close(struct run *run);
void run_init(struct run *run, unsigned int index);
