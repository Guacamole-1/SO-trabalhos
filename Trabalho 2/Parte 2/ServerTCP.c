#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>
#include "sockets.h"
#include "globals.h"

void *t_worker(){
    // make threads
    // send threads to accept
}
//TODO testar se tudo funciona
int main(int argc, char const *argv[])
{
    //printf("%ld",sizeof(argv) / sizeof(argv[0]));

    char* out_file = NULL;
    packet* startcmd = get_cmd(argv,argc,out_file);
    if (!startcmd)
    {
        exit(0);
    }
    
    exec_cmd(startcmd,NULL);
    exit(0);
    signal(SIGPIPE, SIG_IGN);
    log_init("./logfile.txt");
    int server_fd = tcp_server_socket_init(1234);
    
    int p = fork();
    if (p == 0)
    {
        int serv;
        packet msg;
        msg.run = "<program>";
        msg.args = "<argumentos>";
        msg.file = "<filename>";
        msg.dim = 2;
        msg.data = "ab";
        
        
        if ((serv=tcp_client_socket_init("127.0.0.1",1234))){
            int a = smsg(serv,startcmd);
            // send_all(serv,msg,strlen(msg));
            // char* lds = "ab";
            // int a = send_all(serv,lds,strlen(lds));
            // sleep(1);
            printf("SERVER: %d\n",a);
        }  
    }
    else 
    {
        int clientfd = tcp_server_socket_accept(server_fd);
        
        rmsg(clientfd);
        wait(NULL);
        shutdown(clientfd,SHUT_RDWR);
        close(server_fd);
    }
    
    return 0;
}

