/*Initialization of Mini-shell(msh) process*/
void init()
{
        MSH_PID = getpid();
		/*getpid() returns the process ID of the calling process
		header files to be included for this are sys/types.h and unistd.h
		*/
        MSH_TERMINAL = STDIN_FILENO;
		/*MSH_Terminal is static int STDIN_FILENO is a file descriptor 
		used with lower level functions like read().*/
        MSH_IS_INTERACTIVE = isatty(MSH_TERMINAL);
        /*isatty function returns 1 if filedes is a file descriptor associated 
		with an open terminal device, and 0 otherwise.
		isatty() call used to determine if the program is being run interactively
		A subshell that runs interactively has to ensure that it has been placed in
		the foreground by its parent shell before it can enable job control itself.
		It does this by getting its initial process group ID with the getpgrp 
		function, and comparing it to the process group ID of the current 
		foreground job associated with its controlling terminal 
		(which can be retrieved using the tcgetpgrp function).
		If the subshell is not running as a foreground job, 
		it must stop itself by sending a SIGTTIN signal to its own process group.
		It may not arbitrarily put itself into the foreground; 
		it must wait for the user to tell the parent shell to do this. 
		If the subshell is continued again, it should repeat the check and stop 
		itself again if it is still not in the foreground.
		Once the subshell has been placed into the foreground by its 
		parent shell, it can enable its own job control. 
		It does this by calling setpgid to put itself into its own process group,
		and then calling tcsetpgrp to place this process group into the foreground.
		This is being done in the following code*/
		if (MSH_IS_INTERACTIVE) {
                while (tcgetpgrp(MSH_TERMINAL) != (MSH_PGID = getpgrp()))
                        kill(MSH_PID, SIGTTIN);
				/*tcgetpgrp(int filedes) Gets the process group ID (PGID) 
				of the foreground process group associated with the terminal 
				specified by fildes. 
			    The kill() function sends a signal to a process or process group 
				specified by pid. 
		        The signal to be sent is specified by sig and is either 
				0 or one of the signals from the list in the 
				<sys/signal.h> header file*/
                signal(SIGQUIT, SIG_IGN);
                signal(SIGTTOU, SIG_IGN);
                signal(SIGTTIN, SIG_IGN);
                signal(SIGTSTP, SIG_IGN);
                signal(SIGINT, SIG_IGN);
                signal(SIGCHLD, &signalHandler_child);

                setpgid(MSH_PID, MSH_PID);
                MSH_PGID = getpgrp();
                if (MSH_PID != MSH_PGID) {
                        printf("Error, the shell is not process group leader");
                        exit(EXIT_FAILURE);
                }
                if (tcsetpgrp(MSH_TERMINAL, MSH_PGID) == -1)
                        tcgetattr(MSH_TERMINAL, &MSH_TMODES);

                currentDirectory = (char*) calloc(1024, sizeof(char));
        } else {
                printf("Could not make MSH interactive. Exiting..\n");
                exit(EXIT_FAILURE);
        }
}

void welcomeScreen()
{
        printf("\n___________________________________________________________\n");
        printf("\tWelcome to MINI-shell version \n");
        puts("\tAuthors :Kartik and Aakash");
        printf("____________________________________________________________\n");
        printf("\n\n");
}

void shellPrompt()
{
        printf("%s \\m/ ",getcwd(currentDirectory, 1024));
		/*Prompt symbol of our shell is '\m/'
		The getcwd function(in unistd.h) returns an absolute file name representing the 
		current working directory, storing it in the character array buffer(currentDirectory)
		that we provide.The size argument is how we tell the system the allocation 
		size of buffer.
		*/
}


void getTextLine()//get user's command
{
        destroyCommand();//delete previous command from processing buffer
        while ((userInput != '\n') && (bufferChars < BUFFER_MAX_LENGTH)) {
                buffer[bufferChars++] = userInput;
                userInput = getchar();
        }
        buffer[bufferChars] = 0x00;//it means a NULL pointer.Trying to access this data raises a Segmentation Fault
        populateCommand();
}

void destroyCommand()//to be used in getTextLine
{
        while (commandArgc != 0) {
                commandArgv[commandArgc] = NULL;
                commandArgc--;
        }
        bufferChars = 0;
}

void populateCommand()
/*breaks the user stored text in buffer appropriately and stores that in global 
array of strings commandArgc
*/ 
{
        char* bufferPointer;
        bufferPointer = strtok(buffer, " ");
		/*char *strtok(char *str, const char *delim) 
		breaks string str into a series of tokens using the delimitrer delim.*/
        while (bufferPointer != NULL) {
                commandArgv[commandArgc] = bufferPointer;//points to bufferpointer string
                bufferPointer = strtok(NULL, " ");//collect next token in buffer pointer
                commandArgc++;
        }
}

void handleUserCommand()
{
/*check whether command belongs to built in commands defined by us*/
        if (checkBuiltInCommands() == 0) {
                launchJob(commandArgv, "STANDARD", 0, FOREGROUND);                              
        }
}

int checkBuiltInCommands()
{
        if (strcmp("exit", commandArgv[0]) == 0) {//exit from terminal
                exit(EXIT_SUCCESS);
        }
        if (strcmp("cd", commandArgv[0]) == 0) {//change the directory

                changeDirectory();//defined below
                return 1;
        }
        if (strcmp("bg", commandArgv[0]) == 0) {//start a job in background
                if (commandArgv[1] == NULL)
                        return 0;
                if (strcmp("in", commandArgv[1]) == 0)
                        launchJob(commandArgv + 3, *(commandArgv + 2), STDIN, BACKGROUND);
                else if (strcmp("out", commandArgv[1]) == 0)
                        launchJob(commandArgv + 3, *(commandArgv + 2), STDOUT, BACKGROUND);
                else
                        launchJob(commandArgv + 1, "STANDARD", 0, BACKGROUND);
                return 1;
        }
        if (strcmp("fg", commandArgv[0]) == 0) {
                if (commandArgv[1] == NULL)
                        return 0;
                int jobId = (int) atoi(commandArgv[1]);
                t_job* job = getJob(jobId, BY_JOB_ID);
                if (job == NULL)
                        return 0;
                if (job->status == SUSPENDED || job->status == WAITING_INPUT)
                        putJobForeground(job, TRUE);
                else                                                                                                // status = BACKGROUND
                        putJobForeground(job, FALSE);
                return 1;
        }
        if (strcmp("jobs", commandArgv[0]) == 0) {
                printJobs();
                return 1;
        }
        if (strcmp("kill", commandArgv[0]) == 0)
        {
                if (commandArgv[1] == NULL)
                        return 0;
                
                killJob(atoi(commandArgv[1]));
                return 1;
        }
        return 0;

}

void changeDirectory()//to change the current working directory
{
        if (commandArgv[1] == NULL) {
                chdir(getenv("HOME"));
				/*in stdlib.h The C library function char *getenv(const char *name)
				searches for the environment string pointed to by name 
				and returns the associated value to the string.*/
        } else {
                if (chdir(commandArgv[1]) == -1) {
                        printf(" %s: no such directory\n", commandArgv[1]);
					/*The chdir() function makes the directory named by path 
					the new current directory.
					If the chdir() function fails, the current directory is 
					unchanged.returns -1 when fails
					*/
                }
        }
}

void launchJob(char *command[], char *file, int newDescriptor,
               int executionMode)
{
        pid_t pid;
        pid = fork();
		/*In unistd.h int fork() turns a single process into 2 identical processes,
		known as the parent and the child.
		On success, fork() returns 0 to the child process and 
		returns the process ID of the child process to the parent process. 
		On failure,fork() returns -1 to the parent process, 
		sets errno to indicate the error, and no child process is created. 
		*/
        switch (pid) {
        case -1:
                perror("MSH");
                exit(EXIT_FAILURE);
                break;
        case 0://executing child process
                /*
				Macros like SIGINT, SIGQUIT, etc are defined in <signal.h> 
				header file for common signals. 
				*/
				signal(SIGINT, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                signal(SIGCHLD, &signalHandler_child);
                signal(SIGTTIN, SIG_DFL);
                setpgrp();
				/*If the process is not already a session leader, 
				setpgrp() sets the process group ID of the calling process 
				to the process ID of the calling process.*/
                if (executionMode == FOREGROUND)
                        tcsetpgrp(MSH_TERMINAL, getpid());
				
                if (executionMode == BACKGROUND)
                      printf("[%d] %d\n", ++numActiveJobs, (int) getpid());

                //to execute a command
				executeCommand(command, file, newDescriptor, executionMode);

                exit(EXIT_SUCCESS);
                shellPrompt();
                break;
        default://executing parent process
                setpgid(pid, pid);

                //insert the job in global array of job list being maintained
				jobsList = insertJob(pid, pid, *(command), file, (int) executionMode);

                t_job* job = getJob(pid, BY_PROCESS_ID);

                if (executionMode == FOREGROUND) {
                        putJobForeground(job, FALSE);
                }
                if (executionMode == BACKGROUND)
                        putJobBackground(job, FALSE);
                break;
        }
}
void executeCommand(char *command[], char *file, int newDescriptor,
                    int executionMode)
{
        int commandDescriptor;
        if (newDescriptor == STDIN) {
                commandDescriptor = open(file, O_RDONLY, 0600);
				/*O_RDONLY for read only mode
				Upon successful completion, the function shall open the file 
				and return a non-negative integer representing the lowest numbered
				unused file descriptor. Otherwise, -1 shall be returned and
				errno set to indicate the error.
				No files shall be created or modified if the function returns -1.
				*/
                dup2(commandDescriptor, STDIN_FILENO);
				/*system call
				Using dup2(), we can redirect standard output to a file
				dup2 returns the value of the second parameter (fildes2) upon success. 
				A negative return value means that an error occured.
				*/
                close(commandDescriptor);
        }
        if (newDescriptor == STDOUT) {
                commandDescriptor = open(file, O_CREAT | O_TRUNC | O_WRONLY, 0600);
                dup2(commandDescriptor, STDOUT_FILENO);
                close(commandDescriptor);
        }
        if (execvp(*command, command) == -1)
                perror("MSH");
}


//insert a job in a global array of jobList
t_job* insertJob(pid_t pid, pid_t pgid, char* name, char* descriptor,
                 int status)
{
        usleep(10000);
        t_job *newJob = malloc(sizeof(t_job));

        newJob->name = (char*) malloc(sizeof(name));
        newJob->name = strcpy(newJob->name, name);
        newJob->pid = pid;
        newJob->pgid = pgid;
        newJob->status = status;
        newJob->descriptor = (char*) malloc(sizeof(descriptor));
        newJob->descriptor = strcpy(newJob->descriptor, descriptor);
        newJob->next = NULL;

        if (jobsList == NULL) {
                numActiveJobs++;
                newJob->id = numActiveJobs;
                return newJob;
        } else {
                t_job *auxNode = jobsList;
                while (auxNode->next != NULL) {
                        auxNode = auxNode->next;
                }
                newJob->id = auxNode->id + 1;
                auxNode->next = newJob;
                numActiveJobs++;
                return jobsList;
        }
}
//to get pointer to requested job as per the choice
t_job* getJob(int searchValue, int searchParameter)
{
        usleep(10000);
        t_job* job = jobsList;
        switch (searchParameter) {
        case BY_PROCESS_ID:
                while (job != NULL) {
                        if (job->pid == searchValue)
                                return job;
                        else
                                job = job->next;
                }
                break;
        case BY_JOB_ID:
                while (job != NULL) {
                        if (job->id == searchValue)
                                return job;
                        else
                                job = job->next;
                }
                break;
        case BY_JOB_STATUS:
                while (job != NULL) {
                        if (job->status == searchValue)
                                return job;
                        else
                                job = job->next;
                }
                break;
        default:
                return NULL;
                break;
        }
        return NULL;
}


void putJobForeground(t_job* job, int continueJob)
{
        job->status = FOREGROUND;
        tcsetpgrp(MSH_TERMINAL, job->pgid);
        if (continueJob) {
                if (kill(-job->pgid, SIGCONT) < 0)
				//If pid is less than -1, 
				//then sig is sent to every process in the process group whose ID is -pid. […]"
                        perror("kill (SIGCONT)");
        }

        waitJob(job);
        tcsetpgrp(MSH_TERMINAL, MSH_PGID);
}

void waitJob(t_job* job)
{
        int terminationStatus;
        
		/*The waitpid() function shall not suspend execution of the calling thread 
		if status is not immediately available for one of the child processes 
		specified by pid*/
		while (waitpid(job->pid, &terminationStatus, WNOHANG) == 0) {
                if (job->status == SUSPENDED)
                        return;
        }
        jobsList = delJob(job);
}

void killJob(int jobId)
{
        printf("The job ID %d", jobId);
        t_job *job = getJob(jobId, BY_JOB_ID);
        kill(job->pid, SIGKILL);
        jobsList = jobsList->next;
}

t_job* delJob(t_job* job)
{
        usleep(10000);
        if (jobsList == NULL)
                return NULL;
        t_job* currentJob;
        t_job* beforeCurrentJob;

        currentJob = jobsList->next;
        beforeCurrentJob = jobsList;

        if (beforeCurrentJob->pid == job->pid) {

                beforeCurrentJob = beforeCurrentJob->next;
                numActiveJobs--;
                return currentJob;
        }

        while (currentJob != NULL) {
                if (currentJob->pid == job->pid) {
                        numActiveJobs--;
                        beforeCurrentJob->next = currentJob->next;
                }
                beforeCurrentJob = currentJob;
                currentJob = currentJob->next;
        }
        return jobsList;
}


void putJobBackground(t_job* job, int continueJob)
{
        if (job == NULL)
                return;

        if (continueJob && job->status != WAITING_INPUT)
                job->status = WAITING_INPUT;
        if (continueJob)
                if (kill(-job->pgid, SIGCONT) < 0)
                        perror("kill (SIGCONT)");

        tcsetpgrp(MSH_TERMINAL, MSH_PGID);
}


int changeJobStatus(int pid, int status)//to change status attribute of a job with pid
{
        usleep(10000);
        t_job *job = jobsList;
        if (job == NULL) {
                return 0;
        } else {
                int counter = 0;
                while (job != NULL) {
                        if (job->pid == pid) {
                                job->status = status;
                                return TRUE;
                        }
                        counter++;
                        job = job->next;
                }
                return FALSE;
        }
}


void printJobs()//prints the global jobList array
{
        printf("\nActive jobs:\n");
        printf(
                "---------------------------------------------------------------------------\n");
        printf("| %7s  | %30s | %5s | %10s | %6s |\n", "job no.", "name", "pid",
               "descriptor", "status");
        printf(
                "---------------------------------------------------------------------------\n");
        t_job* job = jobsList;
        if (job == NULL) {
                printf("| %s %62s |\n", "No Jobs.", "");
        } else {
                while (job != NULL) {
                        printf("|  %7d | %30s | %5d | %10s | %6c |\n", job->id, job->name,
                               job->pid, job->descriptor, job->status);
                        job = job->next;
                }
        }
        printf(
                "---------------------------------------------------------------------------\n");
}


