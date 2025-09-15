/****************
LE2: Introduction to Unnamed Pipes
****************/
#include <unistd.h> // pipe, fork, dup2, execvp, close
using namespace std;

int main () {
    // lists all the files in the root directory in the long format
    char* cmd1[] = {(char*) "ls", (char*) "-al", (char*) "/", nullptr};
    // translates all input from lowercase to uppercase
    char* cmd2[] = {(char*) "tr", (char*) "a-z", (char*) "A-Z", nullptr};

    // TODO: add functionality
    // Create pipe
    int pipefd[2];
    pipe(pipefd);
    pid_t pipe_1 = fork();
    // child 1:
    // close read end, redirect the stdout to the write end of the pipe 
    if(pipe_1 == 0){
        close(pipefd[0]);
	dup2(pipefd[1], STDOUT_FILENO);
	close(pipefd[1]);
	execvp(cmd1[0], cmd1);
    }
    
    pid_t pipe_2 = fork();
    // child 2: 
    // close the write end, redirect the stdin to the read end of the pipe
    if(pipe_2 == 0){
	close(pipefd[1]);
	dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        execvp(cmd2[0], cmd2);
    }
    
    // parent
    close(pipefd[0]);
    close(pipefd[1]);
    
    return 0;
}    
