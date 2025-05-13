#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "sockets.h"
#include "globals.h"




// @return -1 em erro ou fd do servidor em sucesso
int tcp_server_socket_init (int serverPort){
    int server_fd = -1;
    char msg[100];

    struct sockaddr_in server = {AF_INET, htons(serverPort), {inet_addr("0.0.0.0")}}; 
    server_fd = create_server((struct sockaddr*)&server,sizeof(server));
    if( server_fd != -1){
        sprintf(msg,"TCP server started on port %d",serverPort);
        log_message(INFO,msg);

    } else{
        close(server_fd);
        server_fd = -1;
    }

    return server_fd;
}

// @return -1 em erro ou fd do cliente em sucesso
int tcp_server_socket_accept (int serverSocket){

    return server_socket_accept(serverSocket,"tcp");

}
// @return -1 em erro ou fd do server em sucesso
int tcp_client_socket_init (const char *host, int port)
{
    int server_fd = -1;

    struct sockaddr_in server = {AF_INET, htons(port), {inet_addr(host)}};
    
    CHECK_ERROR((server_fd = socket(AF_INET, SOCK_STREAM, 0)));

    if( connect(server_fd, (struct sockaddr*)&server, sizeof(server)) != -1){
        log_message_width_end_point(INFO , "Connected to TCP server",server_fd);
        return server_fd;
    } else{
        close(server_fd);
        return -1;
    }
}


// @return -1 em erro ou fd do servidor em sucesso
int un_server_socket_init (const char *serverEndPoint){
    int server_fd = -1;
    char msg[100];

    struct sockaddr_un server = create_un_socket(serverEndPoint);

    unlink(serverEndPoint);
    server_fd = create_server((struct sockaddr*)&server,sizeof(server));

    if( server_fd != -1){
        sprintf(msg,"UNIX server started on %s",serverEndPoint);
        log_message(INFO,msg);

    } else{
        unlink(serverEndPoint);
        close(server_fd);
        server_fd = -1;
    }

    return server_fd;
}
// @return -1 em erro ou fd do cliente em sucesso
int un_server_socket_accept (int serverSocket){
    return server_socket_accept(serverSocket,"unix");
}

// @return -1 em erro ou fd do server em sucesso
int un_client_socket_init (const char *serverEndPoint){

    int server_fd = -1;

    struct sockaddr_un server = create_un_socket(serverEndPoint);
    
    CHECK_ERROR((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)));

    if( connect(server_fd, (struct sockaddr*)&server, sizeof(server)) != -1){
        log_message_width_end_point(INFO , "Connected to UNIX server",server_fd);
        return server_fd;
    } else{
        close(server_fd);
        return -1;
    }
}

int server_socket_accept(int serverSocket,char* conn_type){
    int connfd;
    char log_msg[100];

    if( !CHECK_ERROR(connfd = accept(serverSocket, NULL, NULL))){
        snprintf(log_msg,sizeof(log_msg),"New %s connection established",conn_type);
        log_message_width_end_point(INFO , log_msg ,connfd);
        return connfd;
    }
    
    return -1;
}

//criar um listener(server) com socket addr
//@return fd do servidor em sucesso ou -1 em erro
int create_server(const struct sockaddr *addr, socklen_t addrlen) {
    int errors = 0;
    int server_fd = -1;

    if (addr->sa_family == AF_UNIX) {
        const char *path = ((const struct sockaddr_un*)addr)->sun_path;
        unlink(path);
    }

    if(CHECK_ERROR((server_fd = socket(addr->sa_family, SOCK_STREAM, 0)))){
        return -1;
    }

    if(CHECK_ERROR(bind(server_fd, addr, addrlen))){
        close(server_fd);
        return -1;
    }

    if(CHECK_ERROR(listen(server_fd, SERVER_BACKLOG))){
        close(server_fd);
        return -1;
    }

    return (errors == 0) ? server_fd : -1;
}

struct sockaddr_un create_un_socket(const char *serverEndPoint){
    struct sockaddr_un socket;
    memset(&socket, 0, sizeof(socket));              
    socket.sun_family = AF_UNIX;                      
    strncpy(socket.sun_path, serverEndPoint, sizeof(socket.sun_path) - 1);
    return socket;
}


