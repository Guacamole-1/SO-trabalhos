#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "sockets.h"
#include "globals.h"
#include "packets.h"

#define TCP_PORT 1234
#define UNIX_PATH "/tmp/my_server_socket"

// struct para listener 
typedef struct {
    int server_fd; // fd do servidor
    int (*accept_function)(int); // função para aceitar conexões (tcp ou unix)
} listenerArgs;


void* handle_connection(void* arg) {
    int clientfd = *(int*)arg;
    free(arg);

    packet* received = rmsg(clientfd);
    if (received != NULL)
    {
        packet* execd = exec_cmd(received);
        smsg(clientfd,execd);
    }
    


    log_message_width_end_point(INFO, "Connection closed", clientfd);
    close(clientfd);
    return NULL;
}

// listener genérico
void* listener(void* arg) {
    listenerArgs* args = (listenerArgs*)arg;
    while (1) {
        int conn_fd = args->accept_function(args->server_fd);
        if (conn_fd < 0) continue;

        int* fd_ptr = malloc(sizeof(int));
        if (!fd_ptr) {
            log_error("malloc");
            close(conn_fd);
            continue;
        }
        *fd_ptr = conn_fd;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_connection, fd_ptr);
        pthread_detach(tid);
    }

    return NULL;
}

int main() {
    int tcp_fd, un_fd;

    log_init("server.log");
     
    if ((tcp_fd = tcp_server_socket_init(TCP_PORT)) == -1) {
        log_message(ERROR, "Failed to start TCP server");
        return 1;
    }

    if ((un_fd = un_server_socket_init(UNIX_PATH)) == -1) {
        log_message(ERROR, "Failed to start UNIX server");
        return 1;
    }


    listenerArgs tcp_args = { tcp_fd, tcp_server_socket_accept };
    listenerArgs unix_args = { un_fd, un_server_socket_accept };


    pthread_t tcp_thread, unix_thread;
    pthread_create(&tcp_thread, NULL, listener, &tcp_args);
    pthread_create(&unix_thread, NULL, listener, &unix_args);

    log_message(INFO, "TCP and UNIX servers running");

    pthread_join(tcp_thread, NULL);
    pthread_join(unix_thread, NULL);

    return 0;
}
