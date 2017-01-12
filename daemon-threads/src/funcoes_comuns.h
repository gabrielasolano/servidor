#ifndef FUNCOES_COMUNS_H
#define FUNCOES_COMUNS_H

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <sys/queue.h>
#include <sys/un.h>

#define BUFFERSIZE BUFSIZ
#define MAXCLIENTS 150
#define NUM_THREADS 50
#define SOCK_PATH "echo_socket"
#define CONFIG_PATH "config.txt"
#define PID_PATH "pid.txt"

/*! Struct para controle dos clientes ativos */
typedef struct Monitoramento
{
	FILE *fp;
	int sock;
	int quit;
	int recebeu_cabecalho;
	int tam_cabecalho;
	unsigned long frame_recebido;
	unsigned long frame_escrito;
	unsigned long tam_arquivo;
	unsigned long bytes_por_envio;
	unsigned long bytes_enviados; /*! Get: bytes enviados, Put: bytes recebidos*/
	unsigned long bytes_lidos; /*! Get: bytes lidos, Put: bytes escritos */
	unsigned long pode_enviar; /*! Bytes para enviar sem estourar a banda */
	char caminho[PATH_MAX+1];
	char cabecalho[BUFFERSIZE+1];
	struct timeval t_cliente;
} monitoramento;

/*! Mutex e condition para cada cliente */
typedef struct Cliente_Thread
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} cliente_thread;

void escreve_arquivo_config(int porta, char diretorio[], long banda_maxima);
void escreve_arquivo_pid();

extern int controle_velocidade;
extern int quit;
extern int alterar_config;
extern long buffer_size;
extern long banda_maxima;
extern char diretorio[PATH_MAX+1];
extern char config_path[PATH_MAX+1];
extern char pid_path[PATH_MAX+1];
extern char sock_path[PATH_MAX+1];
extern int ativos ;
extern fd_set read_fds;
extern struct Monitoramento clientes[MAXCLIENTS];
extern struct Cliente_Thread clientes_threads[MAXCLIENTS];
extern pthread_cond_t condition_master;
extern pthread_mutex_t mutex_master;
extern pthread_mutex_t mutex_fila_request_get;
extern pthread_mutex_t mutex_fila_response_get;
extern pthread_mutex_t mutex_fila_request_put;

#endif
