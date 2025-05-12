#include "packets.h"
#include <sys/stat.h>
#include <stdio.h>

int execpak(packet* pak){



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
            pak->data = data;
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
    if (strcmp(argv[1],"convert") != 0 && strcmp(argv[1],"pdftotext") != 0 && strcmp(argv[1],"receive") != 0){
        snprintf(log_msg,sizeof(log_msg),"Command not recognised: %s",argv[1]);;
        goto log_error;
    }
    strcpy(pak->run,argv[1]);    

    // meter os argumentos em pak->args
    // snprintf para segurança de memória, para não escrever mais que ARGS_SIZE
    int n = 0;
    for (int i = 2; i < argc-1; i++) {
        n += snprintf(pak->args+n,ARGS_SIZE-n, "%s%s",argv[i], (i != argc - 2) ? " " : "");
    }
    if (n >= ARGS_SIZE){
        log_message(ERROR,"Exceeded argument size! try increasing ARGS_SIZE");
        goto free_pak;
    }
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
    //printf("in_file: %s \nout_file: %s\n",in_file,out_file);
    //printf("!!%d-%d!!", in_status, out_status );
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
    
    
    // alocar espaço para a data do ficheiro
    pak->dim = in.st_size;
    pak->data = malloc(pak->dim);
    if(!pak->data){
        snprintf(log_msg,sizeof(log_msg),"failed to allocate memory: %s",strerror(errno));
        goto log_error;
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

    //printf("size: %d\n",pak->dim); 
    //printf("%s",pak->data);
    //printf("run: %s\nargs: %s",pak->run,pak->args);
    //fwrite(pak->data,1,pak->dim,stdout);
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
int read_all(int sockfd, char *buf, size_t len) {
    size_t   total = 0;
    ssize_t recvd = 0;
    while (total < len) {
        if(CHECK_ERROR(recvd = read(sockfd, buf + total, len - total)))
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
