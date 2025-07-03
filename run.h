#define RUN_STATUS_UNKNOWN 0x3a834e55
#define RUN_STATUS_PENDING 0xa4d263ea
#define RUN_STATUS_ABORTED 0x1b434e26
#define RUN_STATUS_PASSED 0x143d1a45
#define RUN_STATUS_FAILED 0xfce3ea6a

struct run
{

    unsigned int index;
    int stderrfd;
    int stdoutfd;

};

int run_prepare(struct run *run, struct log *log);
int run_update_remote(struct run *run, struct log *log, char *remote);
int run_update_status(struct run *run, struct log *log, unsigned int status);
int run_get_pid(struct run *run, struct log *log);
int run_update_pid(struct run *run, struct log *log, unsigned int pid);
int run_open(struct run *run, struct log *log);
int run_close(struct run *run);
void run_init(struct run *run, unsigned int index);
