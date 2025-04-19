#ifndef SOCKETS_H
#define SOCKETS_H


#define SERVER_BACKLOG 1 //não é muito importante o backlog se estivermos sempre a aceitar conexões

int tcp_server_socket_init (int serverPort);
int tcp_server_socket_accept (int serverSocket);
int tcp_client_socket_init (const char *host, int port);
int un_server_socket_init (const char *serverEndPoint);
int un_server_socket_accept (int serverSocket);
int un_client_socket_init (const char *serverEndPoint);

#endif