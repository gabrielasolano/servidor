#include "funcoes_comuns.h"

/*! Variaveis globais utilizadas por diversas funcoes */
char config_path[PATH_MAX+1];
char pid_path[PATH_MAX+1];
char sock_path[PATH_MAX+1];
int controle_velocidade;
int quit = 0;
int alterar_config = 0;
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

void escreve_arquivo_config (int porta, char diretorio[], long banda_maxima)
{
	FILE *fp;

	fp = fopen(config_path, "w");
	fprintf(fp, "%d\n%s\n%ld", porta, diretorio, banda_maxima);
	fclose(fp);
}

void escreve_arquivo_pid ()
{
	FILE *fp;
	pid_t pid_servidor;

	pid_servidor = getpid();

	fp = fopen(pid_path, "w");
	fprintf(fp, "%d\n", pid_servidor);
	fclose(fp);
}
