#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <errno.h> 
// Macro para dar a _check_error a função que o chamou (em string), o
// ficheiro que a chamou e a linha
// Retorna: 1-Erro 0-Sucesso
#define CHECK_ERROR(expr) _check_error((expr), #expr, __FILE__, __LINE__, 0)
#define CHECK_ERROR_E(expr) _check_error((expr), #expr, __FILE__, __LINE__, 1)

#define DEBUG_MODE 1

int _check_error(int result, const char* func, const char* file, const int line,const int exit_failure);

typedef enum {INFO, ERROR, DEBUG} LOG_LEVEL;


int log_init (const char *pathname);
int log_message(LOG_LEVEL level, const char *msg);
int log_message_width_end_point(LOG_LEVEL level, const char *msg, int sock);
int log_close ();
void log_error(char* cmd_error);

#endif