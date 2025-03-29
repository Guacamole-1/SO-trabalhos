#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void getError(){
    perror("Erro:");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("ERROR!\nUsage: %s [file]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // bytes escritos em pipefd[1] são lidos em pipefd[0]
    int pipefd[2];
    pid_t pid;

    if (pipe(pipefd) == -1 || (pid = fork()) == -1  ) {
        getError();
    }

    if (pid == 0) { //filho
        close(pipefd[0]);

        // Redireciona o stdout para o pipe
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            getError();
        }
        close(pipefd[1]);

        execlp("wc", "wc", "-w", argv[1], NULL);
        getError();
    } else { // pai
        
        close(pipefd[1]); 
        char buffer[1024];
        int n;

        // Lê os dados enviados pelo filho e imprime no stdout
        while ((n = read(pipefd[0], buffer, sizeof(buffer)-1)) > 0) {
            buffer[n] = '\0';
            printf("%s", buffer);
        }
        
        close(pipefd[0]);
        waitpid(pid, NULL, 0); 
    }

    return 0;
}