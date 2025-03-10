#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define NUMBER_OF_CHILDS 3

int main()
{
	for (int i = 0; i < NUMBER_OF_CHILDS; i++)
	{
		pid_t p = fork();
		if(p < 0 )
		{
			perror("ERROR");
			exit(1);
		}
		// processo filho 
		else if ( p == 0)
		{
			for (int i = 0; i < 5; i++)
			{
				printf("Child id: %d\n",getpid());
				printf("Parent id: %d\n",getppid());
				sleep(1);
			}
			exit(0);
		}
	}
	// processo pai
	for (int i = 0; i < NUMBER_OF_CHILDS; i++) //esperar para os processos acabarem
	{
		printf("Child with PID %d has terminated.\n", wait(NULL));
	}

}