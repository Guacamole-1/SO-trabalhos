
#include "globals.h"

#include <time.h>
#include <string.h>

static FILE *log_file = NULL;

// verificar se a função é < 0 e dar print ao erro
// receber as constantes de o nome da função que a chamou, do ficheiro e do numero
// e da linha recebidos do macro CHECK_ERROR
// @return 1 em erro e 0 em sucesso
int _check_error(int result, const char* func, const char* file, const int line,const int exit_failure){
    
    if(result < 0){
        if(DEBUG_MODE){
            fprintf(stderr, "[DEBUG] in (%s:%d) %s:\n", file, line,func);
        }
        log_message(ERROR,strerror(errno));
        if (exit_failure == 1){
            exit(EXIT_FAILURE);
        }
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


const char* log_level_to_str(LOG_LEVEL level) 
{
    switch (level) 
    {
        case INFO:  return "INFO";
        case ERROR: return "ERROR";
        case DEBUG: return "DEBUG";
        default:    return "UNKNOWN";
    }
}

char* get_date(){
    int size = 20;
    char* date = malloc(size);
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(date, size, "%Y-%m-%d %H:%M:%S", t);
    return date;
}


// inicia o ficheiro pathname em modo "w" 
// @return -1 em erro e 0 em sucesso
int log_init (const char *pathname)
{
    log_file = fopen(pathname, "w");

    if (!log_file) {
        char msg[150] = "log file could not be created";
        snprintf(msg, sizeof(msg),"log file could not be created: %s",strerror(errno));
        log_message(ERROR, msg);
        return -1;
    }
    
    if (DEBUG_MODE)
    {
        char msg[150] = "log file sucessfully created in ";
        snprintf(msg, sizeof(msg),"log file successfully created in %s",pathname);
        log_message(DEBUG, msg);
    }
    
    return 0;
}

// dá print ao stdout e ao ficheiro definido em log_init
// @return -1 em erro e 0 em sucesso
int log_message(LOG_LEVEL level, const char *msg){
    int ret = -1;
    char* date = get_date();
    if (level == DEBUG && !DEBUG_MODE) return 0;

    if(log_file != NULL){
        fprintf(log_file,"[%s] %s - %s\n",log_level_to_str(level), date, msg);
        ret = 0;
    }
    printf("[%s] %s - %s\n",log_level_to_str(level), date, msg);

    free(date);
    return ret;
}

// @return string formatado da info da socket
char* get_sock_info(int fd) {
    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    char* return_msg = malloc(150);

    if(CHECK_ERROR(getpeername(fd, (struct sockaddr *)&addr, &len))){
        strcpy(return_msg,"Error");
        return return_msg;
    }

    if (addr.ss_family == AF_INET) {
        struct sockaddr_in *in = (struct sockaddr_in *)&addr;
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &in->sin_addr, ip, sizeof(ip));
        sprintf(return_msg,"IP=%s Port=%d", ip, ntohs(in->sin_port));
    }
    else if (addr.ss_family == AF_UNIX) {
        struct sockaddr_un *un = (struct sockaddr_un *)&addr;
        sprintf(return_msg,"Path: %s", un->sun_path);
    }

    return return_msg;
}

// Igual a log_message mas com informação do socket
// @return -1 em erro e 0 em sucesso
int log_message_width_end_point(LOG_LEVEL level, const char *msg, int sock){
    int ret = -1;
    if(sock == -1) return -1;
    if (level == DEBUG && !DEBUG_MODE) return 0;
    
    char* date = get_date();

    char* sockinfo = get_sock_info(sock);

    if(log_file != NULL){
        fprintf(log_file,"[%s] %s - %s: %s\n",log_level_to_str(level), date, msg, sockinfo);
        ret = 0;
    }
    printf("[%s] %s - %s: %s\n",log_level_to_str(level), date, msg, sockinfo);

    free(date);
    free(sockinfo);
    return ret;
}

int log_close(){
    return CHECK_ERROR(fclose(log_file));
}

void log_error(char* cmd_error){
    char log_msg[200];
    CHECK_ERROR(snprintf(log_msg,sizeof(log_msg),"Error running %s: %s",cmd_error,strerror(errno)));
    log_message(ERROR,log_msg);
}

