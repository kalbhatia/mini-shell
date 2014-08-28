/*The main file that needs to be compiled.Will work in Linux OS only*/
#include <stdio.h>
/*header file containing declarations of functions, macros, struct and other liberary
files containing system calls used*/
#include "declarations.h"
/*definations of user defined functions*/
#include "utilities.h"
#define MAXLINE 4096
int main(int argc, char **argv, char **envp)
{
        init();//begins initializationof mini-shell, see utilities.h
        welcomeScreen();//screen to be displayed when mini shell starts
        shellPrompt();//set the prompt of the shell
        /*Enter an infinite loop*/
		while (TRUE) {
                userInput = getchar();
                switch (userInput) {
                case '\n'://if user presses ENTER key
                        shellPrompt();
                        break;
                default:
                        getTextLine();//accept command from user
                        handleUserCommand();//handles the command obtained 
                        shellPrompt();
                        break;
                }
        }
        printf("\n");
        return 0;
}

void signalHandler_child(int p)//Signal handler for SIGCHLD
{
        pid_t pid;
        int terminationStatus;
        pid = waitpid(WAIT_ANY, &terminationStatus, WUNTRACED | WNOHANG);
        if (pid > 0) {
                t_job* job = getJob(pid, BY_PROCESS_ID);
                if (job == NULL)
                        return;
                if (WIFEXITED(terminationStatus)) {
                        if (job->status == BACKGROUND) {
                                printf("\n[%d]+  Done\t   %s\n", job->id, job->name);
                                jobsList = delJob(job);
                        }
                } else if (WIFSIGNALED(terminationStatus)) {
                        printf("\n[%d]+  KILLED\t   %s\n", job->id, job->name);
                        jobsList = delJob(job);
                } else if (WIFSTOPPED(terminationStatus)) {
                        if (job->status == BACKGROUND) {
                                tcsetpgrp(MSH_TERMINAL, MSH_PGID);
                                changeJobStatus(pid, WAITING_INPUT);
                                printf("\n[%d]+   suspended [wants input]\t   %s\n",
                                       numActiveJobs, job->name);
                        } else {
                                tcsetpgrp(MSH_TERMINAL, job->pgid);
                                changeJobStatus(pid, SUSPENDED);
                                printf("\n[%d]+   stopped\t   %s\n", numActiveJobs, job->name);
                        }
                        return;
                } else {
                        if (job->status == BACKGROUND) {
                                jobsList = delJob(job);
                        }
                }
                tcsetpgrp(MSH_TERMINAL, MSH_PGID);
        }
}
