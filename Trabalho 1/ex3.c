#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#define MSGLEN 100

int main(){

    int pipefd[2];
    int pipeReturn = pipe(pipefd);
    pid_t p = fork();
    if (p == -1 || pipeReturn == -1) {
        perror("Error");
        exit(EXIT_FAILURE);
    }
    else if (p == 0)// processo filho
    {
        close(pipefd[1]); // filho não precisa de enviar data
        int msg, n;
        while (read(pipefd[0],&msg,sizeof(msg)) > 0)
        {
            printf("%d squared is %d\n",msg,msg*msg);
        }
        printf("closed son\n");
        close(pipefd[0]);
    }
     else //processo pai
    {
        close(pipefd[0]); // pai não precisa de ler data
        char msg[MSGLEN];
        int num;
        int r;
        while (1) {
            printf("Enter number: ");
            if((r = scanf("%d",&num)) == EOF)
                break;
            
            if (r > 0)
            {
                write(pipefd[1], &num, sizeof(num));  // Envia o número para o filho
            }else{
                printf("That is not a number!\n");
                scanf("%*s"); // descartar a string invalida
            }
            sleep(0.01);// esperar para o processo filho dar printf para não dar o pai printf primeiro
            
        }
        printf("\nClosed parent\n");
        close(pipefd[1]);
    }
}