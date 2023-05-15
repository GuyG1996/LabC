#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[]){
    //char command [2][10] ;

    int pipefd[2];
    pipe(pipefd);

    int pid;
    fprintf(stderr, "forking first time \n");
    pid = fork();

    if (pid == 0) {//its a child
        //close(pipefd[0]);  // Close unused read end of the pipe
        close(STDOUT_FILENO); // Close unused stdOut
        int writeEndOfPipe = dup(pipefd[1]);
        close(pipefd[1]);
        dup2(STDOUT_FILENO, writeEndOfPipe);
        char * const args[] = {"ls", "-l", NULL};
        fprintf(stderr, "first child excecuting \n");
        execvp("ls", args);
    }

    else{ //parrent procces
        close(pipefd[1]);

        int pid2;
        pid2 = fork();

        if(pid2 == 0){//its a second child
            close(STDIN_FILENO);    // Close unused stdIn
            int readEndOfPipe = dup(pipefd[0]);
            close(pipefd[0]);
            dup2(STDIN_FILENO, readEndOfPipe);
            char * const args2[] = {"tail", "-n", "2", NULL};
            fprintf(stderr, "second child excecuting \n");
            execvp("tail", args2);
        }
        else{ //parent process
            close(pipefd[0]);
        }

    }




return 0;
}