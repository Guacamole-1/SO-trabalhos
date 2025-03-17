#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>



int main ()
{
    printf("Parent process starts (pid = %d; ppid=%d)...\n",
                getpid(), getppid());

    char* date[] = {"date", NULL};
    char* ping[] = {"ping", "-c", "4", "www.google.com",NULL};
    char** arg_list[] = {date,ping};
    int pids[2];
    for (int i = 0; i < 2; i++)
    {
        pid_t p = fork();
        if (p == -1) {
            perror("Error");
            exit(EXIT_FAILURE);
        }
        if (p == 0)
        {
            printf("Child Process with pid = %d; ppid = %d\n", getpid(), getppid());
            pids[i] = getpid();
            execvp(arg_list[i][0],arg_list[i]);
            perror("Error");
            exit(EXIT_FAILURE);
        }
    }


    // processo pai
    printf("My pid = %d and created a process %d and %d\n", getpid(),pids[0],pids[1]);

    for (int i = 0; i < 2; i++)
    {
        int status;
        pid_t pid = waitpid(pids[i],&status,0);
        if (pid == -1) {
            perror("Error");
            exit(EXIT_FAILURE);
        }
        printf("Process pid = %d", pid);
        if ( WIFEXITED(status) ) {
            printf(" has terminated normally with exit value %d\n", WEXITSTATUS(status));
        }
        else if ( WIFSIGNALED(status) ) {
            printf(" has terminated by signal %d\n", WTERMSIG(status));
        }
    }
    
    

    printf("process %d terminating\n", getpid());
    return 0;
}