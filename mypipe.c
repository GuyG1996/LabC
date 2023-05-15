#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

//Ron Hadad 209260645, Guy Ginat 206922544

int main(int argc, char *argv[]){
    int pipefd[2];
    int pid;
    char buffer[256];

    pipe(pipefd);

    pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Fork failed!\n");
        exit(0);
    } else if (pid == 0) {
        // Child process 
        close(pipefd[0]); // Close unused read end of the pipe
        char *message = "Hello";
        write(pipefd[1], message, strlen(message) + 1);
        close(pipefd[1]); // Close write end of the pipe
        exit(0);
    } else {
        // Parent process
        close(pipefd[1]); // Close unused write end of the pipe
        read(pipefd[0], buffer, sizeof(buffer));
        printf("Received message from child process: %s\n", buffer);
        close(pipefd[0]); // Close read end of the pipe
        waitpid(pid, NULL, 0); // Wait for child process to exit
        exit(0);
    }
}    