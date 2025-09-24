#define RUN_STATUS_UNKNOWN 0x3a834e55
#define RUN_STATUS_PENDING 0xa4d263ea
#define RUN_STATUS_ABORTED 0x1b434e26
#define RUN_STATUS_PASSED 0x143d1a45
#define RUN_STATUS_FAILED 0xfce3ea6a

struct run
{

    char *id;
    unsigned int index;
    int stderrfd;
    int stdoutfd;
    int pid;
    unsigned int status;

};

int run_load_pid(struct run *run);
int run_load_status(struct run *run);
int run_save_status(struct run *run);
int run_save_remote(struct run *run, char *remote);
int run_save_pid(struct run *run);
int run_prepare(struct run *run);
void run_print(struct run *run);
void run_printstd(struct run *run, unsigned int descriptor);
int run_openstd(struct run *run);
int run_closestd(struct run *run);
void run_init(struct run *run, char *id, unsigned int index);
