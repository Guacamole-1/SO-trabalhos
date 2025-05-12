#define RUN_SIZE 11
#define ARGS_SIZE 256
#define FILENAME_SIZE 100
#define HEADERLEN RUN_SIZE+ARGS_SIZE+FILENAME_SIZE+sizeof(int)

typedef struct _packet
{
    char* run;  // o programa a ser executado
    char* args; // os argumentos a serem passados ao programa
    char* file; // o nome do ficheiro
    int dim;    // a dimensão em bytes do ficheiro (podia-se usar long int mas
                // com int tem-se um tamanho máximo de 4Gb que é mais que suficiente
    char* data; // conteudo do ficheiro
} packet;

packet* create_packet(char* data,int dim,int fill_data);
packet* get_cmd(const char* argv[],int argc,char* out_file);
int read_all(int sockfd, char *buf, size_t len);
int send_all(int sockfd, char *buf, size_t len);
int smsg(int socketfd,packet* msg);
packet* rmsg(int socketfd);