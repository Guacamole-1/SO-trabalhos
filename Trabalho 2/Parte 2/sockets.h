#ifndef SOCKETS_H
#define SOCKETS_H



#define SERVER_BACKLOG 100 //não é muito importante o backlog se estivermos sempre a aceitar conexões
#define RUN_SIZE 11
#define ARGS_SIZE 256
#define FILENAME_SIZE 100
#define HEADERLEN RUN_SIZE+ARGS_SIZE+FILENAME_SIZE+sizeof(int)



int tcp_server_socket_init (int serverPort);
int tcp_server_socket_accept (int serverSocket);
int tcp_client_socket_init (const char *host, int port);
int un_server_socket_init (const char *serverEndPoint);
int un_server_socket_accept (int serverSocket);
int un_client_socket_init (const char *serverEndPoint);
int server_socket_accept(int serverSocket,char* conn_type);
int create_server(const struct sockaddr *addr, socklen_t addrlen);
struct sockaddr_un create_un_socket(const char *serverEndPoint);
int start_tcp_server(int port);
int start_unix_server(const char *path);

#endif