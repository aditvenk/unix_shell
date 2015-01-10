#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <sys/wait.h>
#include "mysh.h"

void executeBuiltIn (long built_in_index) {
    char* dir;
    switch (built_in_index) {
        case EXIT:
            //expression syntax check. exit should have no more argument. 
            if (argsCount != 1) {
                printError();return;
            }
            exit(0); assert(0);
        case CD: 
            //need to cd to location passed in arguments
            // cd expects only two args
            if (argsCount > 2) {
                printError(); return;
            }
            else if (argsCount == 1) {
                //special case, need to chdir to $HOME
                if (chdir(getenv("HOME")) == -1) {
                    printError(); return;
                }
            }
            else { // chdir to dir passed as arg
                dir = strdup(argsList[1]);
                if (chdir(dir) == -1) {
                    printError(); return;
                }
            }
            break;
        case PWD:
            //expression syntax check. pwd should have no more argument. 
            if(argsCount != 1) {
                printError(); return;
            }
            //execute pwd
            char buf[MAX_INPUT_LENGTH];
            if(getcwd(buf, MAX_INPUT_LENGTH) != NULL)
                printf("%s\n", buf);
            else {
                printError(); return;
            }
            break;
        default:
                return;
    }
    return;
}

int main (int argc, char** argv) {
    char* tmp_input;
    long i; //iteration variable
    long pid1 = -1, pid2 = -1;
    char* nl_pos;
    long built_in_index;
    int status1, status2;
    int pipefd[2];
    
    //making a copy of stdout
    int stdout_copy = dup(fileno(stdout));

    input_line = malloc (sizeof(char)*MAX_INPUT_LENGTH);
    if (input_line == NULL) {
        printError();
        exit(1);
    }
    while (1) {
        
        pipe(pipefd); //creating the kernel pipe
        //close opened files. 
        if( op != NULL) {
            fclose(op);
            op = NULL;
            dup2(stdout_copy, 1);
        }

        printf("mysh> ");
        if (fgets(input_line, MAX_INPUT_LENGTH, stdin) == NULL) {
            printError(); continue;
        }
        if(!strcmp("\n", input_line))
            continue;
        
        //remove newline
        nl_pos = strchr(input_line, '\n');
        if (nl_pos != NULL)
            *nl_pos = '\0';
        
        //get count of args and store 
        tmp_input = strdup(input_line);
        argsCount = countArgCmd(tmp_input);
        
        //fetch args and store
        argsList = malloc(sizeof(char*)*(argsCount+1)); //we allocate for argsCount+1 to account for last NULL. 
        args2 = (char **) malloc(sizeof(char *) * (argsCount+1));
        redir_pos = -1; // if there is redirection in cmdline, this will hold it. note, we use same index variable for redirections & pipes.
        pid1 = -1;
        pid2 = -1;

        if(argsList == NULL || args2 == NULL) {
            perror("Unable to allocate memory!"); exit(1);
        }
        for(i=0; i < argsCount; i++) {
            argsList[i] = malloc(sizeof(char)*MAX_INPUT_LENGTH);
            if(argsList[i] == NULL) {
                perror("Unable to allocate memory!"); exit(1);
            }
            tmp_input = strdup(input_line);
            argsList[i] = fetchArg(tmp_input, i);
        }
        
        // cmd is interpreted as below. note that first token is considered as COMMAND-NAME
        // mysh> <COMMAND-NAME> <ARG0> <ARG1> ... 
        
        //first check for redirection
        isRedirect = checkRedirect();
        if (isRedirect != NO_REDIRECT && isRedirect != PIPE) {
            //the op file is at posn (redir_pos + 1)
            op = fopen(argsList[redir_pos+1], (isRedirect==WRITE)? "w":"a");
            if (op == NULL) {
                printError();
            }
            if (dup2(fileno(op), 1) == -1) {
                printError();
            }
            // remove redirection & filename from args passed to command. 
            argsCount = redir_pos; 
        }
            
        // check if cmd is built in cmd
        EXECUTE_BUILD_IN()
        
        
        // looks like command is not a built in command. 
        // the way we will handle this is this:
        // 1) first fork a new process
        // 2) in the child, do any book-keeping tasks that might be required for the new process. eg) supporting redirection, pipes etc. 
        // 3) then child will call the execvp
        // 4) meanwhile, parent will wait for the child to complete and die. (wait or waitpid)
        
       
        handlePipe(isRedirect);

        pid1 = fork();
        if (pid1 < 0) {
            printError(); continue;
        }
        else if (pid1 == 0) { //child1
            //first check if there is a pipe
            if (isRedirect == PIPE) {
                dup2(pipefd[1], 1);
                close(pipefd[0]);
                close(pipefd[1]);
            }
            child1();
        }
        else { //parent
            if (isRedirect == PIPE) {
                pid2 = fork();//fork again to create process for executing second program
                if (pid2 < 0) {
                    printError(); exit(1);
                }
                else if (pid2 == 0) { //child2
                    dup2(pipefd[0], 0);
                    close(pipefd[1]);
                    close(pipefd[0]);
                    child2();
                }
            }
            close(pipefd[0]);
            close(pipefd[1]);
            if (waitpid(pid1, &status1, 0) == -1)
                printError();
            if (pid2 != -1) {
                if (waitpid(pid2, &status2, 0) == -1)
                    printError();
            }
        }
        free(args2);
        free(argsList);
    }
    close(stdout_copy);
    free(input_line);
    return 0;
}
