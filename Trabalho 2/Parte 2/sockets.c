#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "sockets.h"
#include "globals.h"

//! TODO: MAKE SOCKET_CLOSE() CLI & SERVER
//! make a packet from header
//! make variaridic log_message



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

// @return -1 em erro ou fd do servidor em sucesso
int tcp_server_socket_init (int serverPort){
    int server_fd = -1;
    char msg[40];

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

    int connfd;

    if( !CHECK_ERROR(connfd = accept(serverSocket, NULL, NULL))){
        log_message_width_end_point(INFO , "New connection established",connfd);
        return connfd;
    }

    return -1;

}
// @return -1 em erro ou fd do server em sucesso
int tcp_client_socket_init (const char *host, int port)
{
    int server_fd = -1;

    struct sockaddr_in server = {AF_INET, htons(port), {inet_addr(host)}};
    
    CHECK_ERROR((server_fd = socket(AF_INET, SOCK_STREAM, 0)));

    if( !CHECK_ERROR(connect(server_fd, (struct sockaddr*)&server, sizeof(server)))){
        log_message_width_end_point(INFO , "Connected to TCP server",server_fd);
        return server_fd;
    } else{
        close(server_fd);
        return -1;
    }
}

struct sockaddr_un create_un_socket(const char *serverEndPoint){
    struct sockaddr_un socket;
    memset(&socket, 0, sizeof(socket));              
    socket.sun_family = AF_UNIX;                      
    strncpy(socket.sun_path, serverEndPoint, sizeof(socket.sun_path) - 1);
    socket.sun_path[sizeof(socket.sun_path)] = '\0';
    return socket;
}

int un_server_socket_init (const char *serverEndPoint){
    int server_fd = -1;
    char msg[26 + strlen(serverEndPoint)];

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
int un_server_socket_accept (int serverSocket){
    int connfd;

    if( !CHECK_ERROR(connfd = accept(serverSocket, NULL, NULL))){
        log_message_width_end_point(INFO , "New connection established",connfd);
        return connfd;
    }
    
    return -1;
}
int un_client_socket_init (const char *serverEndPoint){

    int server_fd = -1;

    struct sockaddr_un server = create_un_socket(serverEndPoint);
    
    CHECK_ERROR((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)));

    if( !CHECK_ERROR(connect(server_fd, (struct sockaddr*)&server, sizeof(server)))){
        log_message_width_end_point(INFO , "Connected to UNIX server",server_fd);
        return server_fd;
    } else{
        close(server_fd);
        return -1;
    }
}

//TODO fazer uma função para receber o estado

//* vai ser executado pelo server
// executa o comando em pak
// retorna 0 em sucesso e -1 em erro
packet* exec_cmd(packet* pak,char* output_file){
    char log_msg[100];
    int status, exit_code;
    int sspipe[2]; //pipe para os filhos
    int sfpipe[2]; // pipe do filho para o pai
    pid_t p1,p2;
    int args_count = 5; // em principio já tem 1 arg, 
                        // mais o arg NULL, o arg do comando e os ficheiros in and out
    if (pipe(sspipe) == -1 || pipe(sfpipe) == -1)
    {
        log_error("pipe");
        return NULL;
    }
    if ( (p1 = fork()) < 0 )
    {
        log_error("fork");
        return NULL;
    } else if (p1 == 0) // processo filho (mandar a data do ficheiro para o filho 2 (p2) )
    {
        //fechar pipes, estes serão usados para comunicação do p2 para pai
        close(sfpipe[0]); close(sfpipe[1]);
        close(sspipe[0]); // não precisa de ler

        //mandar data para o filho 2
        write(sspipe[1],pak->data,pak->dim);
        
        close(sspipe[1]);
        exit(0);
    }
    
    
    if ( (p2 = fork()) < 0 )
    {
        log_error("fork2");
        return NULL;
    }
    else if ( p2 == 0 ){  // processo filho (receber do filho 1 (p1) fazer exec e mandar ao pai)
        // filho não precisa de ler o pai
        close(sfpipe[0]);
        // filho não precisa de escrever para filho 1
        close(sspipe[1]);
        int _arg = 0;

        if (dup2(sspipe[0], STDIN_FILENO) == -1) {
            log_error("dup2");
            exit(1);
        }
        if (dup2(sfpipe[1], STDOUT_FILENO) == -1) {
            log_error("dup2");
            exit(1);
        }
        // duplicar stderr para se houver erros mandar para o pipe
        if (dup2(sfpipe[1], STDERR_FILENO) == -1) {
            log_error("dup2");
            exit(1);
        }

        close(sspipe[0]);
        close(sfpipe[1]);

        // contar os espaços
        for (int i = 0; pak->args[i] != '\0'; i++) {
            if (pak->args[i] == ' ') {
                args_count++;
            }
        }
        char* args[args_count];

        // meter os args em um array
        args[_arg++] = pak->run;
        for(char *token = strtok(pak->args, " "); token != NULL ; token = strtok(NULL, " ")){
            args[_arg++] = token;            
        }
        // dizer ao exec que vai receber o input ao stdin e mandar o output para stdout
        args[_arg++] = "-";
        args[_arg++] = "-";
        args[_arg] = NULL;

        execvp(pak->run,args);
        log_error("exec");
        exit(0);

    }
        // processo pai
        close(sspipe[1]); close(sspipe[0]);
        close(sfpipe[1]);
        
        int buffer_len = 0;
        int temp_size = 4096;
        char *buffer = malloc(temp_size);
        if(!buffer) log_error("malloc");
        // ler o que vem do filho 2 e alocar memoria como for preciso
        while (1) {
            ssize_t bytes_read = read(sfpipe[0], buffer + buffer_len, temp_size);

            if (bytes_read < 0) {
                log_error("read");
                free(buffer);
                return NULL;
            } else if (bytes_read == 0) {
                break; // EOF
            }

            buffer_len += bytes_read;

            buffer = realloc(buffer, buffer_len + temp_size);
            if (!buffer) {
                log_error("realloc");
                exit(1);
            }
        }

        waitpid(p1, NULL, 0);
        waitpid(p2, &status, 0);
        write(STDOUT_FILENO,buffer,buffer_len);
        //printf("what: %d",buffer_size);
        // verificar se o comando foi executado corretamente
        if (WIFEXITED(status)) {
            exit_code = WEXITSTATUS(status);
            if (exit_code == 0) {
                snprintf(log_msg,sizeof(log_msg),"%s succeeded",pak->run);
                log_message(DEBUG,log_msg);
            } else {
                snprintf(log_msg,sizeof(log_msg),"%s failed with exit code %d",pak->run, exit_code);
                log_message(ERROR,log_msg);
            }
        } else {
            printf("exited abnormally with code\n");
            snprintf(log_msg,sizeof(log_msg),"%s exited abnormally with status %d",pak->run, status);
            log_message(ERROR,log_msg);
            exit_code = status;
        }
        
        packet* result = create_packet(buffer,buffer_len,1);
        
        strcpy(result->run,"receive");
        snprintf(result->args,ARGS_SIZE,"%d",status);
        strcpy(result->file,"\0"); // output file vai ser determinado pelo cliente
        result->dim = buffer_len;
        strncpy(result->data,buffer,buffer_len);

        free(buffer);

        return result;
        // get from exec

}

// dá free a um packet p, se has_data == 1 apaga p->data
// retorna 0 em sucesso e -1 em erro
int free_packet(packet* p,int has_data){

    if (p)
    {
       if(has_data)
            free(p->data);
       free(p->args);
       free(p->file);
       free(p->run);
       free(p);
       return 0;
    }
    
    return -1;
}

// criar packet, se fill_data == 1, preencher com data
// retorna packet* em sucesso e NULL em erro
packet* create_packet(char* data,int dim,int fill_data){

    packet *pak = malloc(sizeof(packet));
    if(!pak) goto ret;
    // +1 para espaço para o '\0'
    pak->run = malloc(RUN_SIZE+1);
    if (!pak->run) goto free_pak;

    pak->args = malloc(ARGS_SIZE+1);
    if (!pak->args) goto free_run;
    
    pak->file = malloc(FILENAME_SIZE+1);
    if (!pak->file) goto free_args;

    if(fill_data){
        if (data == NULL || dim <= 0) goto free_file;
        pak->dim = dim;
        pak->data = malloc(dim); // não precisa de +1 porque são bytes
        if (!pak->data) goto free_file;
        memcpy(pak->data, data, dim);
        
    }

    return pak;

    free_file:
        free(pak->file);
    free_args:
        free(pak->args);
    free_run:
        free(pak->run);
    free_pak:
        free(pak);
    ret:
        CHECK_ERROR(-1);
     	return NULL;
}

packet* get_cmd(const char* argv[],int argc,char* out_file){

    int mallocd_data = 0; // variavel para ver se pak tem data alocada para free_pak dar free a data também
    char log_msg[100];
    char* in_file;
    int files_count = 0;
    struct stat in;
    struct stat out;
    int in_status = -1;
    int out_status = -1;
    FILE *file_stream;

    packet* pak = create_packet(NULL,0,0); 
    if(!pak) goto ret;

    // ver se o comando para run existe, e se existir meter em pak 
    if (strcmp(argv[1],"convert") != 0 && strcmp(argv[1],"pdftotext") != 0){
        snprintf(log_msg,sizeof(log_msg),"Command not recognised: %s",argv[1]);;
        goto log_error;
    }
    strcpy(pak->run,argv[1]);    

    //ver quais são os ficheiros de input e output
    for (int i = 2; i < argc; i++)
    {
        if(files_count == 0 && (in_status = stat(argv[i], &in)) == 0 ){
            in_file = argv[i];
            files_count++;
        }else if (files_count == 1 && (out_status = stat(argv[i], &out)) == 0 )
        {
            out_file = argv[i];
            files_count++;
        }
    }

    // check de erros, in_file tem de existir e ser menor que FILENAME_SIZE
    if (in_file == NULL)
    {
        snprintf(log_msg,sizeof(log_msg),"%s: No input file found!",argv[1]);
        goto log_error;
    }
    else if (strlen(in_file) >= FILENAME_SIZE) //|| strlen(out_file) >= FILENAME_SIZE
    {
        log_message(ERROR," Input filename is too big! try increasing FILENAME_SIZE");
        goto free_pak;
    } 
    else
    {
        strncpy(pak->file,in_file,FILENAME_SIZE);  
    }

    // meter os argumentos em pak->args
    int n = 0;
    for (int i = 2; i < argc-1; i++) {
        if (argv[i] != in_file && argv[i] != out_file)
        {
            n += snprintf(pak->args+n,ARGS_SIZE-n, "%s%s",argv[i], (i != argc - 2) ? " " : "");
        }
    }
    if (n >= ARGS_SIZE){
        log_message(ERROR,"Exceeded argument size! try increasing ARGS_SIZE");
        goto free_pak;
    }
    
    // alocar espaço para a data do ficheiro
    pak->dim = in.st_size;
    pak->data = malloc(pak->dim);
    if(!pak->data){
        log_error("malloc");
        goto free_pak;
    } else {
        mallocd_data = 1;
    }

    //abrir ficheiro e por os conteudos em pak->data e depois fechar
    file_stream = fopen(pak->file,"r");
    if (!file_stream){
        snprintf(log_msg,sizeof(log_msg),"failed to open %s :%s",pak->file,strerror(errno));
        goto log_error;
    }
    n = fread(pak->data,1,pak->dim,file_stream);
    if(n != pak->dim){
        snprintf(log_msg,sizeof(log_msg),"failed to read %s :%s",pak->file,strerror(errno));
        fclose(file_stream);
        goto log_error;
    }
    fclose(file_stream);

    return pak;
    
    // labels para se der erro
    log_error:
        log_message(ERROR,log_msg);
    free_pak:
        free_packet(pak,mallocd_data);
    ret:
     	return NULL;

}
// tenta receber toda a informação
// retorna -1 em erro e 0 em sucesso
int read_all(int fd, char *buf, size_t len) {
    size_t   total = 0;
    ssize_t recvd = 0;
    while (total < len) {
        if(CHECK_ERROR(recvd = read(fd, buf + total, len - total)))
        {
            if (errno == EINTR){    // interrompido por sinal
                continue;
            }
            return -1;             
        }
        if (recvd == 0) { // conexão fechou ou EOF           
            break;
        }
       
        total += recvd;
    }
    return total;
}

// tenta enviar toda a informação
// retorna -1 em erro e 0 em sucesso
int send_all(int sockfd, char *buf, size_t len) {
    size_t total = 0;

    while (total < len) {
        ssize_t sent = send(sockfd, buf + total, len - total, 0);
        if (CHECK_ERROR(sent)){

            if (errno == EINTR) { // interrompido por sinal
                continue;   
            }
            return -1;
        }
        if (sent == 0) {
            break;         // conexão fechou ou EOF
        }
        total += sent;
    }

    return len == total ? 0 : -1 ;
}

// envia msg para socketfd
// retorna 0 em sucesso e -1 em erro
int smsg(int socketfd,packet* msg){
    int n;
    char header[HEADERLEN];

    if(CHECK_ERROR(n = snprintf(header, HEADERLEN,
        "RUN:  %s\n"
        "ARGS: %s\n"
        "FILE: %s\n"
        "DIM:  %d\n\n",
        msg->run, msg->args, msg->file, msg->dim
    ))){
        return -1;
    }

    if(CHECK_ERROR(n = send_all(socketfd,header,HEADERLEN))){
        return -1;    
    }

    if(CHECK_ERROR(n = send_all(socketfd,msg->data,msg->dim))){
        return -1;
    }
    return 0;
}

// receber msg 
// retorna packet em sucesso e NULL em erro 
packet* rmsg(int socketfd){
    int mallocd_data = 0;
    char log_msg[50];
    int n;

    char* header = malloc(HEADERLEN + 1);// +1 para '\0'
    if (!header) goto return_error; 


    // Ler o header
    // Desta maneira temos um header fixo de 371 bytes
    // fazer receber dinamicamente chunks seria uma melhor solução
    // fazer se houver tempo no fim
    if ((n = read_all(socketfd,header,HEADERLEN)) == -1){
        goto free_buff;
    }
    header[n] = '\0';
    
    sprintf(log_msg,"%d bytes of header received from",n);
    log_message_width_end_point(DEBUG,log_msg,socketfd);

    packet* pak = create_packet(NULL,0,0);

    // macros para se for preciso mudar tamanhos
    // transforma a a constante definida no #define para str
    // dois strings literais adjacentes são concatenados pelo compilador 
    n = sscanf(header,
        "RUN:  %" STR(RUN_SIZE) "[^\n]\n"
        "ARGS: %" STR(ARGS_SIZE)"[^\n]\n"
        "FILE: %" STR(FILENAME_SIZE)"[^\n]\n"
        "DIM:  %d\n\n",
        pak->run, pak->args, pak->file, &pak->dim
    );

    if (n == 4) {
        printf("program = \"%s\"\n", pak->run);
        printf("arguments = \"%s\"\n", pak->args);
        printf("filename = \"%s\"\n", pak->file);
        printf("size = \"%d\"\n", pak->dim);
    } else{
        log_message_width_end_point(ERROR,"Invalid header received from",socketfd);
        goto free_pak;
    }
    
    pak->data = malloc(pak->dim); 
    if (!pak->data) goto free_pak;
    else mallocd_data = 1;

    if ((n = read_all(socketfd,pak->data,pak->dim)) == -1){
        goto free_pak;
    }

    if (n < pak->dim) {
        log_message_width_end_point(ERROR,"File data could not be loaded",socketfd);
        goto free_pak;
    } else
    {
        sprintf(log_msg,"%d bytes of file data received from",n);
        log_message_width_end_point(DEBUG,log_msg,socketfd);
    }
    printf("Packet received:\n");
    fwrite(pak->data,1,pak->dim,stdout);
    
    return pak;

    // labels para se der erro
    free_pak:
        free_packet(pak,mallocd_data);
    free_buff:
        free(header);
    return_error:
        CHECK_ERROR(-1);
        return NULL;
}
