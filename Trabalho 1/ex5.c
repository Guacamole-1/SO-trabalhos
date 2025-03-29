#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#define BUFFERSIZE 255


void getError(){
    perror("Erro:");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
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
        
        for (int i = 1; i < argc; i++)
        {
            printf("%s\n",argv[i]);
        }
        
    } else { // pai
        
        close(pipefd[1]); 
        char buffer[1024];
        int n;
        
        // Lê os dados enviados pelo filho e imprime no stdout
        while ((n = read(pipefd[0], buffer, sizeof(buffer)-1)) > 0) {
            
            buffer[n] = '\0';

        }
        close(pipefd[0]);
        waitpid(pid, NULL, 0);
        
        //wc_args size = argc - argv[0] + "wc" + "-w" = argc + 1
        int args_size = argc+1;
        char *wc_args[args_size];
        int arg_count = 0;
        wc_args[arg_count++] = "wc";
        wc_args[arg_count++] = "-w";
        
        //separar os argumentos enviados pelo filho
        char *files = strtok(buffer, "\n");
        
        while (files != NULL && arg_count < args_size) {
            wc_args[arg_count++] = files;
            files = strtok(NULL, "\n");
        }
        wc_args[arg_count] = '\0';
        
        execvp("wc",wc_args);
        
    }

    return 0;
}