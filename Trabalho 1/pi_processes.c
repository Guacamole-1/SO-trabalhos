#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <math.h>

typedef struct
{
   long long int start;
   long long int finish;
}Range;

double calcPi(Range worker){

    double result;
    for (int i = worker.start; i <= worker.finish; i++)
    {
        result += pow(-1,i+1)/(2*i - 1);
    }
    return 4*result;
}


Range* DivideWork(int terms,int workersNum){

    long long int base = terms / workersNum;
    long long int remainder = terms % workersNum;

    long long int start = 1;
    Range* workers = malloc(workersNum*sizeof(Range));
    
    for (int i = 0; i < workersNum; i++) 
    {
        workers[i].start = start;    
        workers[i].finish = start + base-1;    
        if (i < remainder) // adicionar restos 
        {
            workers[i].finish++;
        }
        start = workers[i].finish + 1;
    }
    return workers;
    
}

void getError(){
    perror("Erro");
    exit(EXIT_FAILURE);
}

int main(int argc, char const *argv[])
{
    long long int terms;
    int workersNum;
    // bytes escritos em pipefd[1] são lidos em pipefd[0]
    int pipefd[2];
    pid_t pid;

    if (argv[2] == NULL)
    {
        argv[2] = "1";
    }

    if (argc < 2 || argc > 3)
    {
        printf("ERROR!\nUsage: %s [n of terms] [n of processes(default=1)]\n", argv[0]);
        exit(EXIT_FAILURE);
    } else if (!(terms = atoll(argv[1])))
    {
        printf("Argument %s is not a valid number!\nUsage: %s [n of terms] [n of processes(default=1)]\n",argv[1], argv[0]);
        exit(EXIT_FAILURE);
    } else if (!(workersNum = atoi(argv[2])))
    {    
        printf("Argument %s is not a valid number!\nUsage: %s [n of terms] [n of processes(default=1)]\n",argv[2], argv[0]);
        exit(EXIT_FAILURE);
    } else if ( terms < (long long int)pow(10, 10))
    {
        printf("Please choose a number bigger than 10^10 !");
        exit(EXIT_FAILURE);
    }
    
    
    
    Range* workers = DivideWork(terms,workersNum);
        
    if (pipe(pipefd) == -1)
        getError();

    // criar processos filho e dar-lhes os calculos
    for (int i = 0; i < workersNum; i++)
	{
		pid_t p = fork();
        double w_sum;
		if (p < 0 )
            getError();
		// processo filho 
		else if ( p == 0)
		{
            // filho não precisa de ler
            close(pipefd[0]);
            // calcular a sua parte
			w_sum = calcPi(workers[i]);
            ssize_t n;
            // mandar para o processo filho
            if((n = write(pipefd[1],&w_sum,sizeof(w_sum))) != sizeof(w_sum)){
                getError();
            }
            close(pipefd[1]);
			exit(0);
		}
	}
    // processo pai
    pid_t w;
    double temp;
    double result;
    close(pipefd[1]);
    ssize_t n;
    //esperar para os processos filhos acabarem
	for (int i = 0; i < workersNum; i++) 
	{
        if((n = read(pipefd[0],&temp,sizeof(temp))) != sizeof(temp)){
            getError();
        }
        result += temp;
        w = wait(NULL);
        if (w == -1)
            getError();
	}
    close(pipefd[0]);
    printf("π = %0.9f",result);

    free(workers);
    return 0;
}
