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
int background_command(int i, char** arglist);
int piping(int count, char** arglist, int ind_of_seperator);
int plain_execution(char** arglist);

// prepare and finalize calls for initialization and destruction of anything required
int prepare(void){
    if (signal(SIGINT, SIG_IGN) == SIG_ERR){
        fprintf(stderr, "Received error during signal\n");
        exit(1);
    }
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR){
        fprintf(stderr, "Received error during signal\n");
        exit(1);
    }
    return 0;
}
int finalize(void){
    return 0;
}


// arglist - a list of char* arguments (words) provided by the user
// it contains count+1 items, where the last item (arglist[count]) and *only* the last is NULL
// RETURNS - 1 if should continue, 0 otherwise
int process_arglist(int count, char** arglist){
    int i;
    for(i = 0; i < count; i++){
        if(arglist[i][0] == '|'){
            /* Piping */
            return piping(count, arglist, i);
        }
        else if(arglist[i][0] == '&'){
            /* Background Commands */
            return background_command(i, arglist);
        }
        else if(arglist[i][0] == '>' && arglist[i][1] == '>'){
            return redirection(count, arglist, i);
        }
    }
    return plain_execution(arglist);
}

int plain_execution(char** arglist){
    int pid = fork();
    if (pid == 0){
        /* Child process */
        if (signal(SIGINT, SIG_DFL) == SIG_ERR){ 
            fprintf(stderr, "Received signal\n");
			exit(1);
		}
        if(execvp(arglist[0], arglist)){
                fprintf(stderr, "Received error!\n");
                return -1;
            }
    }
    else if (pid > 0){
        /* Parent process */
        waitpid(pid, NULL, 0);
        return 1;
    }
    else{
        /* fork failed */
        fprintf(stderr, "Received error during fork\n");
        exit(1);
    }
    return 0;
}

int redirection(int count, char** arglist, int ind_of_seperator){
    arglist[ind_of_seperator] = NULL;
    int pid = fork();
    if (pid == 0){
        /* Child process */
        if (signal(SIGINT, SIG_DFL) == SIG_ERR){ 
            fprintf(stderr, "Received signal\n");
			exit(1);
		}
        int fd;

        if ((fd = open(arglist[ind_of_seperator + 1], O_CREAT | O_APPEND | O_RDWR, 0666)) < 0){
            fprintf(stderr, "Received error during open\n");
            exit(1);
        }
        
        if (dup2(fd, 1) < 0){
            fprintf(stderr, "Received error during dup2\n");
            exit(1);
        }
        
        if (close(fd) < 0){
            fprintf(stderr, "Received error during close\n");
            exit(1);
        }
        
        if(execvp(arglist[0], arglist)){
                fprintf(stderr, "Received error!\n");
                exit(1);
            }
        
    }
    else if (pid > 0){
        /* Parent process */
        waitpid(pid, NULL, 0);
        return 1;
    }
    else{
        /* fork failed */
        fprintf(stderr, "Received error during fork\n");
        exit(1);
    }
    return 0;
}


int piping(int count, char** arglist, int ind_of_seperator){
    arglist[ind_of_seperator] = NULL;
    int pfds[2];
    if (pipe(pfds) < 0) { /* pipe failed */
        fprintf(stderr, "Received pipe error\n");
        return 0;
    }
    
    int pid = fork();

    if (pid > 0){
        /* Parent Process */
        int pid2 = fork();
        if (pid2 > 0){
            /* Parent Process */
            /* closing reading side of pipe */
            if (close(pfds[0]) < 0) { 
            	fprintf(stderr, "Error during close\n");
        		exit(1);
           	}
           	/* closing write side of pipe */
            if (close(pfds[1]) < 0) { 
            	fprintf(stderr, "Error during close\n");
        		exit(1);
           	}

            waitpid(pid2, NULL, 0);
            waitpid(pid, NULL, 0);

            return 1;
        }
        else if (pid2 == 0){
            /* The second child process */
            /* This child will execute the second command */
            waitpid(pid, NULL, 0);

            if (signal(SIGINT, SIG_DFL) == SIG_ERR){ 
                fprintf(stderr, "Received signal\n");
                exit(1);
            }
            /*close writing side of pipe */
            if (close(pfds[1]) < 0) { 
            	fprintf(stderr, "Error during close\n");
        		exit(1);
           	}
            /* instead of stdin, read from the pipe */
            
            if (dup2(pfds[0], 0) < 0) { 
            	fprintf(stderr, "Error during dup2\n");
        		exit(1);
           	}
            /*close read side of pipe */
            if (close(pfds[0]) < 0) { 
                    fprintf(stderr, "Received error during close\n");
                    exit(1);
                }
            
            if(execvp(arglist[ind_of_seperator + 1], (char**)(arglist + ind_of_seperator + 1))){
                    fprintf(stderr, "Received error!\n");
                    exit(1);
                }
        
        }
        else{
            /* fork failed */
            fprintf(stderr, "Received error during fork\n");
            exit(1);
        }
        
    }
    else if (pid == 0){
        /* The first child process */
            /* This child will execute the first command */

            if (signal(SIGINT, SIG_DFL) == SIG_ERR){ 
                fprintf(stderr, "Received signal\n");
			    exit(1);
		    }
            /* close read side of pipe */
            if (close(pfds[0]) < 0) { 
                    fprintf(stderr, "Received error during close\n");
                    exit(1);
                }
            
            /* instead of stdout, write to the pipe */
             
            if (dup2(pfds[1], 1) < 0) { 
                    fprintf(stderr, "Received error during dup2\n");
                    exit(1);
                }
            /* close writing side of pipe */
            if (close(pfds[1]) < 0) { 
                    fprintf(stderr, "Received error during close\n");
                    exit(1);
                }
            if(execvp(arglist[0], arglist)<0){
                fprintf(stderr, "Received error during exec\n");
                exit(1);
            }
            
    }
    else{
        /* fork failed */
        fprintf(stderr, "Received error during fork\n");
        exit(1);
    }
    return 0;
}

int background_command(int i, char** arglist){
    int pid = fork();
    if (pid == 0){
        /* Child process */
        if (signal(SIGINT, SIG_IGN) == SIG_ERR){ 
            fprintf(stderr, "Received signal\n");
			exit(1);
		}
        arglist[i] = NULL;
        if(execvp(arglist[0], arglist)){
                fprintf(stderr, "Received error!\n");
                exit(1);
            }
    }
    else if (pid > 0){
        /* The parent */
        return 1;
    }
    else {
        /* fork failed */
        fprintf(stderr, "Received error during fork\n");
        exit(1);
    }

    return 0;

}