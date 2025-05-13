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

#define DEFAULT_TCP_HOST "127.0.0.1"

int build_path(char *buf, size_t bufsz, const char *out_file, int index, const char *original_name)
{
    if (!out_file || !*out_file || !original_name) {
        errno = EINVAL;          /*  <-- add this               */
        return -1;
    }

    const char *base = strrchr(original_name, '/');

    base = base ? base + 1 : original_name;

    int n = snprintf(buf, bufsz, "%s/[%d]%s", out_file, index, base);

    if (n >= bufsz) { errno = ENAMETOOLONG; return -1; }

    return 0;
}

typedef struct {
    int conn_index;
    const char *protocol;
    const char *server_addr; 
    int server_port;        
    packet *pak;
    const char  *out_file;    
} clientArgs;

void* client_worker(void* arg) {
    clientArgs *client = (clientArgs*)arg;
    int serverfd;
    if (strcmp(client->protocol, "tcp") == 0) {
        serverfd = tcp_client_socket_init(client->server_addr, client->server_port);
    } else {
        serverfd = un_client_socket_init(client->server_addr);
    }
    if (serverfd < 0) {
        log_message(ERROR, "Failed to connect");
        return NULL;
    }
    if(smsg(serverfd,client->pak) == -1){
        log_message(ERROR, "Failed to send command");
        return NULL;
    }
    packet* received = rmsg(serverfd);
    if (!received){
        log_message(ERROR, "Failed to receive reply");
        return NULL;
    }


    char temp[10];
    snprintf(temp,sizeof(temp),"_%d",client->conn_index);
    if (client->out_file == NULL)
    {
        client->out_file = received->file;
    }
    char *out_name = add_suffix(client->out_file,temp);
    if (!out_name)
    {
        log_error("add_sufix");
        close(serverfd);
        return NULL;
    }
    FILE* out = fopen(out_name,"wb"); 
    if (fwrite(received->data,1,received->dim,out) == received->dim)
    {   
        char log_msg[100];
        snprintf(log_msg,sizeof(log_msg),"Received file: %s",out_name);
        log_message(INFO,log_msg);
    }
    

    log_message_width_end_point(INFO, "Connection closed", serverfd);
    close(serverfd);
    return NULL;
}

void exit_error(char* msg,char* str){ 
    printf("[ERROR]\n%sUsage: %s <tcp|unix> <host|path> [port] <n connections> <convert|pdftotext> [args...] <input> [output]\n",msg, str);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc < 6) {
        exit_error("",argv[0]);
    }
    const char *protocol = argv[1];
    const char *server_addr = argv[2];
    int curr_arg = 3;
    int server_port = 0;

    if (strcmp(protocol, "tcp") == 0) {
        if (argc < 7) {
            printf("[ERROR]\nUsage for tcp: %s tcp <host> <port> <n connections> <convert|pdftotext> [args...] <input> [output]\n", argv[0]);
            return 1;
        }
        else if((server_port = atoi(argv[curr_arg++])) == 0){
            printf("[ERROR]\nServer port is not a number!\nUsage for tcp: %s tcp <host> <port> <n connections> <convert|pdftotext> [args...] <input> [output]\n", argv[0]);
            return 1;
        }
    } 
    else if (strcmp(protocol, "unix") == 0)
    {
        if (argc < 6) {
            printf("[ERROR]\nUsage for unix: %s unix <path> <n connections> <convert|pdftotext> [args...] <input> [output]\n", argv[0]);
            return 1;
        }
        
    } 
    
    
    int n_conn = atoi(argv[curr_arg++]);
    if (n_conn <= 0) {
        exit_error("Invalid number of connections\n",argv[0]);
    }
    char **cmd_args = &argv[curr_arg];
    const char *out_file = NULL; 
    packet* pak_send = get_cmd(cmd_args,argc-curr_arg,&out_file);
    if (!pak_send){
        log_error("get_cmd");
        return 1;
    }

    log_init("client.log");

    pthread_t *threads = malloc(n_conn * sizeof(pthread_t));
    clientArgs *cliargs = malloc(n_conn * sizeof(clientArgs));
    if (!threads || !cliargs) {
        log_error("malloc");
        return 1;
    }
    
    for (int i = 0; i < n_conn; i++) {
        cliargs[i].conn_index = i + 1;
        cliargs[i].protocol = protocol;
        cliargs[i].server_addr = server_addr;
        cliargs[i].server_port = server_port;
        cliargs[i].out_file = out_file;
        cliargs[i].pak = pak_send;

        if (pthread_create(&threads[i], NULL, client_worker, &cliargs[i]) != 0) {
            log_message(ERROR, "Failed to create thread");
        }
    }

    for (int i = 0; i < n_conn; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(cliargs);
    log_close();

    return 0;
}

