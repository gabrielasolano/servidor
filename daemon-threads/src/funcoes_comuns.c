#include "funcoes_comuns.h"

/*! Variaveis globais utilizadas por diversas funcoes */

int controle_velocidade;
int quit = 0;
long buffer_size = BUFFERSIZE+1;
long banda_maxima;
char diretorio[PATH_MAX+1];
int ativos = 0;
fd_set read_fds;
struct Monitoramento clientes[MAXCLIENTS];
struct Cliente_Thread clientes_threads[MAXCLIENTS];
pthread_cond_t condition_master;
pthread_mutex_t mutex_master;
pthread_mutex_t mutex_fila_request_get;
pthread_mutex_t mutex_fila_response_get;
pthread_mutex_t mutex_fila_request_put;
