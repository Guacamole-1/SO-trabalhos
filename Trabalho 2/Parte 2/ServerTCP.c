#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "sockets.h"


int main(int argc, char const *argv[])
{
    signal(SIGPIPE, SIG_IGN);
    
    int server_fd = tcp_server_socket_init(1234);
    
    int p = fork();
    if (p == 0)
    {
        tcp_client_socket_init("127.0.0.1",1234);    
    }
    else 
    {
        int clientfd = tcp_server_socket_accept(server_fd);
        wait(NULL);
    }
    
    return 0;
}
