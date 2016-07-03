struct job
{

    char *name;
    char *exec;

};

int job_load(struct job *job, char *name);
int job_save(struct job *job);
int job_erase(struct job *job);
void job_copy(struct job *job, char *name);
void job_create(char *name);
void job_list();
void job_remove(struct job *job);
void job_show(struct job *job);
