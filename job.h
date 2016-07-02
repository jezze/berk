struct job
{

    char *name;
    char *exec;

};

int job_load(char *filename, struct job *job);
int job_save(char *filename, struct job *job);
int job_parse(int argc, char **argv);
