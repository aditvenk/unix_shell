
// enums for TRUE and FALSE. commonly used in comparisons
enum {
    FALSE = 0,
    TRUE = 1
};

// enum for FAIL, SUCCESS. commonly used in function returns
enum {
    FAIL = 0,
    SUCCESS = 1
};

enum {
    NO_REDIRECT = -1,
    WRITE = 0,
    APPEND = 1,
    PIPE = 2
};

//Assumption that max input length is 1024. 
#define MAX_INPUT_LENGTH    1024

#define printError()    fprintf(stderr, "Error!\n")

#define EXECUTE_BUILD_IN()   built_in_index = isCmdBuiltIn(argsList[0]); \
                             if(built_in_index >= 0) { \
                                executeBuiltIn (built_in_index); \
                                continue; \
                             } 


// following commands will get executed in shell's process, not new process
// exit 
// cd
// pwd

enum {
    EXIT,
    CD,
    PWD,
    NUM_BUILT_IN_CMDS
};

//important global variables being used

static char* input_line; //user's entry line. maintain this as immutable. use local copies for mutation 
static long argsCount;
static char** argsList; //store each string in cmdline
static char** args2; //args for piped program
static char* built_in_cmds[NUM_BUILT_IN_CMDS] = {"exit", "cd", "pwd"};
static int redir_pos ; //store location of redirection marker in cmd line
long isRedirect;
static FILE* op;


// function that returns built in command index if command is built in, else -1
long isCmdBuiltIn (char* cmd) {
    long i;
    for (i=0; i<NUM_BUILT_IN_CMDS; i++) {
        if (!strcmp(built_in_cmds[i], cmd))
            return i;
    }
    return -1;
}

//count and return num of args
long countArgCmd(char* line) {
    long count = 0;
    char* token = strtok(line, " ");
    for (; token != NULL; token = strtok(NULL, " ")) {
        count++;
    }
    return count;
}

char* fetchArg(char* line, long n) {
    long count = 0;
    char* token = strtok(line, " ");
    for (; token != NULL; token = strtok(NULL, " "), count++) {
        if (count == n)
            return token;
    }
    return line; //this is dicey. returning the line itself if it does not have an 'n'th arg
}

// function that will execute built_in command in same process and return SUCCESS if all okay. note - exit will not return
void executeBuiltIn (long built_in_index);

//function that will check if the cmdline has redirection. It will return 0 for for overwrite, 1 for append, 2 for pipe & -1 for no-redirection. 
long checkRedirect() {
    char* overwrite = ">";
    char* append = ">>";
    char* pipe = "|";
    long i=0;
    for (; i<argsCount; i++) {
        if (!strcmp(argsList[i], overwrite)) {
            redir_pos = i;
            return WRITE;
        }
        if (!strcmp(argsList[i], append)) {
            redir_pos = i;
            return APPEND;
        }
        if (!strcmp(argsList[i], pipe)) {
            redir_pos = i;
            return PIPE;
        }
    }
    return NO_REDIRECT;
}

void handlePipe() {

    if (isRedirect != PIPE)
        return;

    long i, j;
    for(i=redir_pos+1, j=0; i<argsCount; i++, j++) {
        args2[j] = strdup(argsList[i]);
    }
    args2[j] = NULL; //last position needs to be NULL before calling execvp
    argsCount = redir_pos; //last position needs to be NULL before calling execvp
}

void child1() {
   argsList[argsCount] = NULL;
   execvp(argsList[0], argsList);
   printError();
   exit(1); // why this exit? coz, if user enters invalid command, the child process will throw error, but it won't exit. in that case, the next command in the shell will be received by child. we don't want that. 
}

void child2() {
    execvp(args2[0], args2);
    printError();
    exit(1);
}



