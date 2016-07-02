struct job
{

    char name[32];
    char exec[1024];

};

int job_load(char *job_name, struct job *job);
int job_parse(int argc, char **argv);
