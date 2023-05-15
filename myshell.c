#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include "LineParser.h"
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdbool.h>
//Ron Hadad 209260645, Guy Ginat 206922544

//SETUP:
typedef struct process{
    cmdLine* cmd;                         /* the parsed command line*/
    pid_t pid; 		                  /* the process id that is running the command*/
    int status;                           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next;	                  /* next process in chain */
} process;
#define TERMINATED -1
#define RUNNING 1
#define SUSPENDED 0
const char* getStatusName(int status) {
    const char* name;
    switch (status) {
        case TERMINATED:
            name = "TERMINATED";
            break;
        case RUNNING:
            name = "RUNNING";
            break;
        case SUSPENDED:
            name = "SUSPENDED";
            break;
        default:
            name = "UNKNOWN";
            break;
    }
    return name;
}
//process list functions:
void addProcess(process** process_list, cmdLine* cmd, pid_t pid){
    //  building the new process
    process *newProc = (process *)malloc(sizeof(process));
    newProc->cmd = cmd;
    newProc->pid = pid;
    newProc->status = RUNNING;
    //  making the switch with the head
    newProc->next = *process_list;
    *process_list = newProc;
}
void processDelete(process* process_list, int pid){ //do not use, create segment fault.
    process* curr;
    while((process_list != NULL) && (process_list->pid != pid)){
        curr = process_list;
        process_list = process_list->next;
    }// when done, 'curr' will hold the process that points the right process we're looking for.
    if(process_list == NULL){
        return;
    }
    else{
        curr->next = process_list->next;
        free(process_list);
    }
}
void updateProcessList(process **process_list){
    int status;
    int pid;
    process *curr = *process_list;
    while (curr != NULL) {
        pid = waitpid(curr->pid, &status, WNOHANG);
        if (pid == -1) {
            printf("pid in the list but does not exist\n");
        }
        else if (pid == 0) {
            // Process is still running. nothing to update.
        } 
        else {
            printf("the ststus of %d is %d", curr->pid, status);
            if (WIFEXITED(status)) {    // Process terminated normally
                curr->status = TERMINATED;
            } else if (WIFSIGNALED(status)) {   // Process terminated due to unhandled signal
                curr->status = TERMINATED;
            } else if (WIFSTOPPED(status)) {    // Process stopped due to signal                
                curr->status = SUSPENDED;
            } else if (WIFCONTINUED(status)) {  // Process resumed execution                
                curr->status = RUNNING;
            } 
        }
        curr = curr->next;
    }   
}
void deleteAllTerminatedProcc(process **process_list){
    process *curr = *process_list;
    process *prev = NULL;
    while(curr != NULL){
        if(strcmp(getStatusName(curr->status), "TERMINATED") == 0){ // we found "terminated"
            if(prev == NULL){   //its the first process in the list. 
                *process_list = curr->next;
            }else{
                prev->next = curr->next;
            }
            //  update the 'curr' and free memory
            process *toDelete = curr;
            curr = curr->next;
            free(toDelete);
        }else{
            prev = curr;
            curr = curr->next;    
        }
    }
}
//printing:
void printProcessRecursive(process* process){
    if(process == NULL){
        return;
    }
    else{
        printf("PID                 Command                 STATUS\n");
        printf("%d                ", process->pid);
        printf("%s                  ", process->cmd->arguments[0]);
        printf("%s\n", getStatusName(process->status));
    }
    printProcessRecursive(process->next);
}
void printProcessList(process** process_list){
    updateProcessList(process_list);
    printProcessRecursive(*process_list);
    deleteAllTerminatedProcc(process_list);
}
//maintainig:
void freeProcessList(process* process_list){
    if(process_list == NULL){
        return;
    }
    freeProcessList(process_list->next);
    freeCmdLines(process_list->cmd);
    free(process_list);
}
void updateProcessStatus(process* process_list, int pid, int status){
    while((process_list != NULL) && (process_list->pid != pid)){
        process_list = process_list->next;
    }
    if(process_list == NULL){
        return;
    }
    else{
        process_list->status = status;
    }
}


/*
the commandLineHistory array will hold the commands from the last one (lastCommandIdx), to the previes 20's.
the newCommandIdx always hold the last command.
*/
const int HISTLEN = 20;
char* commandLineHistory[20];
int* newCommandIdx ;
//history functions:
void indexUpdater(){
    *newCommandIdx = *newCommandIdx + 1;
    if(*newCommandIdx == HISTLEN){
        *newCommandIdx = 0;
    }
}
void addCommandToHistory(char* command){
    char* newCommand = (char*)malloc(2048);
    strcpy(newCommand, command);
    indexUpdater();
    commandLineHistory[*newCommandIdx] = newCommand;
}
void printHistory(){
    int currIndx = *newCommandIdx;
    for(int i = 0; i < HISTLEN; i++){
        if(currIndx == -1){
            currIndx = 19;
        }
        if(commandLineHistory[currIndx] == NULL){   //case we dont have anymore history to show
            return;
        }
        else{
            printf("%d : %s", (i + 1), commandLineHistory[currIndx]);
        }
        currIndx--;
    }
}
int getNCommand(char n){
    if(n == '!'){
        return *newCommandIdx;
    }
    int numn = atoi(&n);
    if((numn < 1) || (numn > 20)){
        return -1;
    }
    int indexOfn = *newCommandIdx - numn + 1;//as we start with 0
    if(indexOfn < 0){
        indexOfn = HISTLEN + indexOfn;
    }
    return indexOfn;
}
void freeHistory(){
    for(int i = 0; i < 20; i++){
        if(commandLineHistory[i] != NULL){
            free(commandLineHistory[i]);
        }
    }
    free(newCommandIdx);
}

process** process_list = NULL;
bool debugMode = false;
int pipefd[2];
void execute(cmdLine *pCmdLine){
    if(pCmdLine->next != NULL){// setting the pipe
        if(pCmdLine->outputRedirect != NULL){
            fprintf(stderr, "you can't specipy both chained next file and outputRedirect for first file   \n");
            exit(0);
        }
        if(pCmdLine->next->inputRedirect != NULL){
            fprintf(stderr, "you can't specipy both chained next file and inputRedirect for second file   \n");
            exit(0);
        }
        pipe(pipefd);
    }
    if(strcmp(pCmdLine->arguments[0], "quit") == 0){//case "quit"
        freeCmdLines(pCmdLine);
        freeHistory();
        freeProcessList(*process_list);
        exit(0);
    }
    if(strcmp(pCmdLine->arguments[0], "cd") == 0){//case "cd"
        if (chdir(pCmdLine->arguments[1]) != 0) {//case error chainging directory
            fprintf(stderr, "failed chaing directory \n");
        }
    }
    //special signals for a child procces:
    if(strcmp(pCmdLine->arguments[0], "suspend") == 0){
        int pid = atoi(pCmdLine->arguments[1]);
        if (kill(pid, SIGTSTP) == -1) {
            fprintf(stderr, "failed to suspend process %s \n", pCmdLine->arguments[1]);
        }else{
            updateProcessStatus(*process_list, pid, 0);
        }
        
    }
    else if(strcmp(pCmdLine->arguments[0], "wake") == 0){
        int pid = atoi(pCmdLine->arguments[1]);
        if (kill(pid, SIGCONT) == -1) {
            fprintf(stderr, "failed to wake process %s \n", pCmdLine->arguments[1]);
        }else{
            updateProcessStatus(*process_list, pid, 1);
        }
    }
    else if(strcmp(pCmdLine->arguments[0], "kill") == 0){
        int pid = atoi(pCmdLine->arguments[1]);
        if (kill(pid, SIGTERM) == -1) {
            fprintf(stderr, "failed to kill process %s \n", pCmdLine->arguments[1]);
        }
    }
    else if(strcmp(pCmdLine->arguments[0], "procs") == 0){
        printProcessList(process_list);
    }
    else if(strcmp(pCmdLine->arguments[0], "history") == 0){
        printHistory();
    }    
    else{// if not special signal:
        int pid;
        pid = fork();
        if (pid == 0) {
            // Child process
            if(pCmdLine->next != NULL){//we need to pipe the output
                close(STDOUT_FILENO); // Close unused stdOut
                int writeEndOfPipe = dup(pipefd[1]);
                close(pipefd[1]);
                dup2(STDOUT_FILENO, writeEndOfPipe);
            }
            if(pCmdLine->idx != 0){ //we need to pipe the input
                close(STDIN_FILENO);    // Close unused stdIn
                int readEndOfPipe = dup(pipefd[0]);
                close(pipefd[0]);
                dup2(STDIN_FILENO, readEndOfPipe);
            }

            if(pCmdLine->inputRedirect != NULL){
                int new_stdin = open(pCmdLine->inputRedirect, O_RDONLY);
                if (new_stdin < 0) {
                    fprintf(stderr, "failed to open inputRedirect file  \n");
                    freeCmdLines(pCmdLine);
                    freeHistory();
                    freeProcessList(*process_list);
                    exit(0);
                }
                dup2(new_stdin, STDIN_FILENO);
            }
            if(pCmdLine->outputRedirect != NULL){
                int new_stdout = open(pCmdLine->outputRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (new_stdout < 0) {
                    fprintf(stderr, "failed to open outputRedirect file \n");
                    freeCmdLines(pCmdLine);
                    freeHistory();
                    freeProcessList(*process_list);
                    exit(0);
                }
                dup2(new_stdout, STDOUT_FILENO);
            }
            
            if(execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1){
                fprintf(stderr, "could not execute the command \n");
                freeCmdLines(pCmdLine);
                freeHistory();
                freeProcessList(*process_list);
                exit(0);
            }

        } else if (pid > 0) {
            // Parent process
            addProcess(process_list, pCmdLine, pid);

            if(pCmdLine->next != NULL){
                close(pipefd[1]);
                execute(pCmdLine->next);
            }
            if(pCmdLine->idx != 0){
                close(pipefd[0]);
            }

            if(debugMode){
                fprintf(stderr, "PID : %d \n", pid);
                fprintf(stderr, "Child process executing command : %s \n", pCmdLine->arguments[0]);
            }
            
            if(pCmdLine->blocking == '1'){
                waitpid(pid, NULL, 0);
            }
        } else {
            // Fork failed
            fprintf(stderr, "Fork failed!\n");
            freeCmdLines(pCmdLine);
            freeHistory();
            freeProcessList(*process_list);
            exit(0);
        }
    }
}

int main(int argc, char *argv[]){
    process_list = (process **)malloc(sizeof(process*));
    newCommandIdx = (int*)malloc(sizeof(int));
    *newCommandIdx = -1;
    if(argc>1 && strcmp(argv[1],"-d") == 0){
        debugMode = true;
    }
    while(1){
        char currWrkDrctr[PATH_MAX];
        if (getcwd(currWrkDrctr, sizeof(currWrkDrctr)) != NULL) {
            printf("%s$ ", currWrkDrctr);
            char inputLine[2048];
            if(fgets(inputLine, sizeof(inputLine), stdin) != NULL){
                cmdLine* currcmdLine = parseCmdLines(inputLine);
                if(currcmdLine->arguments[0][0] == '!'){   //in the case of '!' we'll parse and excecute the last n command again.(in case of '!!' n=1)
                    int indexOfn = getNCommand(currcmdLine->arguments[0][1]);
                    if((indexOfn != -1) && (commandLineHistory[indexOfn] != NULL)){
                        currcmdLine = parseCmdLines(commandLineHistory[indexOfn]);
                        addCommandToHistory(commandLineHistory[indexOfn]);
                    }else{
                        fprintf(stderr, "out of range, or no such entry yet \n");
                    }
                }
                else{
                    char* newCommand = (char*)malloc(2048);
                    strcpy(newCommand, inputLine);
                    addCommandToHistory(newCommand);
                    free(newCommand);
                }
                execute(currcmdLine);
            }
            else{
                printf("failed to read stdin");
            }
        }
        else{
            printf("failed to find current working directory");
        }
    }
}