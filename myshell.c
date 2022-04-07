#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h> 
#include <signal.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

int redirection(int count, char** arglist, int ind_of_seperator);
int background_command(int count, char** arglist);
int piping(int count, char** arglist, int ind_of_seperator);


// prepare and finalize calls for initialization and destruction of anything required
int prepare(void){
    if (SIGINT, SIG_IGN) == SIG_ERR){
        fprintf(stderr, "%s\n", "Received error during signal");
        exit(1);
    }
    if (SIGCHLD, SIG_IGN) == SIG_ERR){
        fprintf(stderr, "%s\n", "Received error during signal");
        exit(1);
    }
}
int finalize(void){
    return 0;
}

// arglist - a list of char* arguments (words) provided by the user
// it contains count+1 items, where the last item (arglist[count]) and *only* the last is NULL
// RETURNS - 1 if should continue, 0 otherwise
int process_arglist(int count, char** arglist){
    int i;
    for(i = 0; i <= count; i++){
        if(arglist[i][0] == '|'){
            /* Piping */
            return piping(count, arglist, i);
        }
        else if(arglist[i][0] == '&'){
            /* Background Commands */
            return background_command(count, arglist);
        }
        else if(arglist[i][0] == '>' && arglist[i][1] == '>'){
            return redirection(count, arglist, i);
        }
    }

}

int redirection(int count, char** arglist, int ind_of_seperator){
    int pid = fork();
    if (pid <= 0){
        /* Child process */
        int fd = open(arglist[ind_of_seperator + 1], O_RDWR | O_CREAT | O_APPEND, 666);
        dup2(fd, 1);
        arglist[ind_of_seperator] = NULL;
        if(!execvp(arglist[0], arglist)){
                printf("Error!");
                return -1;
            }
        close(fd);
    }
    else{
        /* Parent process */
        wait(pid);
    }
    return 0;
}


int piping(int count, char** arglist, int ind_of_seperator){
    arglist[ind_of_seperator] = NULL;
    int pid = fork();
    int pfds[2];
    pipe(pfds);
    if (pid > 0){
        /* Parent Process */
        int pid2 = fork();
        if (pid2 > 0){
            /* Parent Process */
            wait(pid2);
            wait(pid);
        }
        else{
            /* The other child process */
            /* This child will execute the first command */
            close(pfds[0]);
            /* instead of stdout, write to the pipe */
            dup2(pfds[1], 1); 
            if(!execvp(arglist[0], arglist)){
                printf("Error!");
                return -1;
            }
            close(pfds[1]);
        }
    }
    else{
        /* The child process */
        /* This child will execute the second command */
        close(pfds[1]);
        /* instead of stdin, read from the pipe */
        dup2(pfds[0], 0);
        if(!execvp(arglist[0], (char**)(arglist + ind_of_seperator))){
                printf("Error!");
                return -1;
            }
        
        close(pfds[1]);
    }
    return 0;
}

int background_command(int count, char** arglist){
    int pid = fork();
    if (pid <= 0){
        /* Child Process */
        arglist[count] = NULL;
        if(!execvp(arglist[0], arglist)){
                printf("Error!");
                return -1;
            }
    }
    else{
        /* The parent */
        
    }

    return 0;

}