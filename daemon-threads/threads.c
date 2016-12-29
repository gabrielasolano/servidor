/*!
 * \file servidor.c
 * \brief Servidor multi-thread interoperando com clientes ativos, com
 * controle de velocidade.
 * \date 27/12/2016
 * \author Gabriela Solano <gabriela.solano@aker.com.br>
 *
 */
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
//#include "func_caminho.h"

#define BUFFERSIZE BUFSIZ
#define MAXCLIENTS 350
#define NUM_THREADS 50
#define PORTA_THREADS 2016

/*!
 * Struct para controle dos clientes ativos.
 */
typedef struct Monitoramento
{
	FILE *fp;
	int sock;
	//int enviar;
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
} monitoramento;

typedef struct Cliente_Thread
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} cliente_thread;

typedef struct Arquivos
{
	char caminho[PATH_MAX+1];
	int arquivo_is_put;
	int socket;
	struct Arquivos *next;
} arquivos;

typedef struct GetRequest
{
	int indice;
	STAILQ_ENTRY(GetRequest) entry;
} get_request;

typedef struct GetResponse
{
	int indice;
	int tam_buffer;
	char buffer[BUFFERSIZE+1];
	STAILQ_ENTRY(GetResponse) entry;
} get_response;

typedef struct PutRequest
{
	int indice;
	int tam_buffer;
	char buffer[BUFFERSIZE+1];
	unsigned long frame;
	STAILQ_ENTRY(PutRequest) entry;
} put_request;

void formato_mensagem();
void inicia_servidor (int *sock, struct sockaddr_in *servidor, const int porta,
											int *sock_thread, struct sockaddr_in *addr_thread);
void zera_struct_cliente (int indice);
void encerra_cliente (int indice);
void verifica_banda (int indice, int ativos);
void cabecalho_put (int indice);
int existe_diretorio (char *caminho);
int grava_arquivo (int indice, int tam_buffer, char *buffer);
void cabecalho_get (int indice);
void recebe_cabecalho_cliente (int indice);
void recupera_caminho (int indice, char *pagina);
int envia_cabecalho (int indice, char cabecalho[], int flag_erro);
int existe_pagina (char *caminho);
int envia_cliente (int sock_cliente, char mensagem[], int size_strlen);
int arquivo_pode_utilizar (int indice, int arquivo_is_put);
void insere_lista_arquivos(int indice, int arquivo_is_put);
void remove_arquivo_lista (int indice);
int recupera_tam_arquivo (int indice);
void cria_threads();
void *funcao_thread(void *id);
void insere_fila_request_get (int indice);
get_request *retira_fila_request_get();
int leitura_arquivo (int indice);
void cabecalho_parser (int indice);
void recebe_mensagem_cliente (int indice);
void encerra_servidor ();
get_response *retira_fila_response_get();
void insere_fila_response_get (char *buf, int indice, int tam_buffer);
void signal_handler (int signum);
void insere_fila_request_put(int indice, char *buf, int tam_buffer,
														 unsigned long frame);
put_request *retira_fila_request_put ();
void recebe_arquivo_put (int indice);
int calcula_tam_alocar (int indice);

struct Monitoramento clientes_ativos[MAXCLIENTS];
struct Monitoramento cliente_vazio = {0};
struct Cliente_Thread clientes_threads[MAXCLIENTS];
struct Arquivos *lista_arquivos = NULL;
char diretorio[PATH_MAX+1];
int ativos = 0;
int controle_velocidade;
int quit = 0;
long banda_maxima;
fd_set read_fds;
struct timeval t_janela;
struct timeval timeout;

pthread_t threads[NUM_THREADS];
pthread_mutex_t mutex_master;
pthread_cond_t condition_master;

STAILQ_HEAD(, GetRequest) fila_request_get;
pthread_mutex_t mutex_fila_request_get;
STAILQ_HEAD(, GetResponse) fila_response_get;
pthread_mutex_t mutex_fila_response_get;
STAILQ_HEAD(, PutRequest) fila_request_put;
pthread_mutex_t mutex_fila_request_put;

/*!
 * \brief Funcao principal que conecta o servidor e processa
 * requisicoes de clientes.
 * \param[in] argc Quantidade e parametros de entrada.
 * \param[in] argv[1] Porta de conexao do servidor.
 * \param[in] argv[2] Diretorio utilizado como root pelo servidor.
 * \param[in] argv[3] Taxa de envio maxima (bytes/segundo) : optativo.
 */
int main (int argc, char **argv)
{
	sigset_t set;
  struct sockaddr_in servidor;
	struct sockaddr_in addr_thread;
	//struct sockaddr_un addr_thread;
  struct sockaddr_in cliente;
  socklen_t addrlen;
	socklen_t addrlen_thread;
  int sock_servidor;
	int sock_thread;
  int sock_cliente;
  int porta;
  int max_fd;
  int i;
	int pedido_cliente;
	char recebido[3];

  /*! Valida quantidade de parametros de entrada */
  if (argc < 3 || argc > 4)
  {
    printf("Quantidade incorreta de parametros.\n");
    formato_mensagem();
    return 1;
  }

  /*! Recupera e valida a porta */
  porta = atoi(argv[1]);
  if (porta < 0 || porta > 65535)
  {
    printf("Porta invalida!\n");
    formato_mensagem();
    return 1;
  }

  /* Recupera a banda_maxima maxima */
	banda_maxima = 0;
	controle_velocidade = 0;
	if (argc == 4)
	{
		controle_velocidade = 1;
		banda_maxima = atol(argv[3]);
		if (banda_maxima <= 0)
		{
			printf("Taxa de envio invalida!\n");
			formato_mensagem();
			return 1;
		}
	}

	/*! Inicia daemon */
	//daemon(1, 0);

  /*! Muda o diretorio do processo para o informado na linha de comando */
  if (chdir(argv[2]) == -1)
  {
    perror("chdir()");
		exit(1);
  }

  /*! Recupera diretorio absoluto */
  getcwd(diretorio, sizeof(diretorio));
  if (diretorio == NULL)
  {
    perror("getcwd()");
		exit(1);
  }

  printf("Iniciando servidor na porta: %d e no path: %s\n", porta, diretorio);

	/*! Configuracao inicial do servidor */
  inicia_servidor(&sock_servidor, &servidor, porta, &sock_thread, &addr_thread);

	for (i = 0; i < MAXCLIENTS; i++)
	{
		zera_struct_cliente(i);
		pthread_mutex_init(&clientes_threads[i].mutex, NULL);
		pthread_cond_init(&clientes_threads[i].cond, NULL);
	}

	timerclear(&t_janela);
	timerclear(&timeout);
	max_fd = sock_servidor;

	/*! Inicializa mutex, condition, e filas */
	pthread_mutex_init(&mutex_master, NULL);
	pthread_cond_init (&condition_master, NULL);

	STAILQ_INIT(&fila_request_get);
	pthread_mutex_init(&mutex_fila_request_get, NULL);
	STAILQ_INIT(&fila_response_get);
	pthread_mutex_init(&mutex_fila_response_get, NULL);
	STAILQ_INIT(&fila_request_put);
	pthread_mutex_init(&mutex_fila_request_put, NULL);

	/*! Signal handler */
	signal(SIGINT, signal_handler);
	sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigprocmask(SIG_UNBLOCK, &set, NULL);

	cria_threads();

  /*! Loop principal para aceitar e lidar com conexoes do cliente */
  while (1)
  {
		FD_ZERO(&read_fds);
		FD_SET(sock_servidor, &read_fds);

    /*! Recebe uma nova conexao */
		if (ativos > 0) /*! Se houve alguma ativa, limita o tempo de espera */
    {
      pedido_cliente = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
    }
    else
    {
			printf("\nEspera de cliente.\n");
      pedido_cliente = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
    }
		/*! Validacao do select */
    if ((pedido_cliente < 0) && (errno != EINTR))
		{
			perror("select()");
			encerra_servidor();
		}

		/*! Adiciona as conexoes ativas no read_fds */
		for (i = 0; i < MAXCLIENTS; i++)
		{
			if (clientes_ativos[i].sock != -1)
			{
				FD_SET(clientes_ativos[i].sock, &read_fds);
				if (clientes_ativos[i].sock > max_fd)
					max_fd = clientes_ativos[i].sock;
			}
		}

		/*! Aceita novas conexoes */
		if ((pedido_cliente > 0) && FD_ISSET(sock_servidor, &read_fds)
					&& (ativos < MAXCLIENTS))
		{
			addrlen = sizeof(cliente);
			sock_cliente = accept(sock_servidor, (struct sockaddr *) &cliente,
														&addrlen);
			if (sock_cliente == -1)
			{
				perror("accept()");
				break;
			}
			else
			{
				/*! Adiciona o novo cliente no array de clientes ativos */
				clientes_ativos[ativos].sock = sock_cliente;
				//clientes_ativos[ativos].enviar = 1;

				/*! Controle da quantidade de clientes ativos */
				ativos++;

				/*! Adiciona o novo socket na read_fds */
				FD_SET(sock_cliente, &read_fds);
				if (sock_cliente > max_fd)
				{
					max_fd = sock_cliente;
				}
				printf("Nova conexao de %s no socket %d\n",
							 inet_ntoa(cliente.sin_addr), (sock_cliente));
			}
		}

		/*! Recebe mensagem das threads */
		addrlen_thread = sizeof(addr_thread);
		while (recvfrom(sock_thread, recebido, sizeof(recebido), 0,
								(struct sockaddr *) &addr_thread, &addrlen_thread) > 0)
		{
			if(!STAILQ_EMPTY(&fila_response_get))
			{
				pthread_mutex_lock(&mutex_fila_response_get);
				get_response *r_aux = retira_fila_response_get();
				pthread_mutex_unlock(&mutex_fila_response_get);

				envia_cliente(r_aux->indice, r_aux->buffer, r_aux->tam_buffer);
				free(r_aux);
			}
		}

		/*! Recebe requests do cliente */
		timerclear(&t_janela);
		timerclear(&timeout);
		for (i = 0; i < MAXCLIENTS; i++)
		{
			if ((clientes_ativos[i].sock != -1)
					&& (FD_ISSET(clientes_ativos[i].sock, &read_fds)))
			{
				if (clientes_ativos[i].recebeu_cabecalho == 0)
				{
					recebe_cabecalho_cliente(i);
				}
				else
				{
					recebe_mensagem_cliente(i);
				}
				if (controle_velocidade && clientes_ativos[i].sock != -1)
				{
					verifica_banda(i, ativos);
				}
			}
		}
  }
  close(sock_servidor);
	close(sock_thread);
  return 0;
}

void signal_handler (int signum)
{
	printf("Recebido sinal %d\n", signum);
	quit = 1;
	encerra_servidor();
}

void cria_threads ()
{
	long i;

	for (i = 0; i < NUM_THREADS; i++)
	{
		if (pthread_create(&threads[i], NULL, funcao_thread, (void *) i) != 0)
		{
			printf("pthread_create() error\n");
			encerra_servidor();
		}
	}
}

get_request *retira_fila_request_get()
{
	struct GetRequest *retorno;

	retorno = STAILQ_FIRST(&fila_request_get);
	STAILQ_REMOVE_HEAD(&fila_request_get, entry);

	return retorno;
}

get_response *retira_fila_response_get()
{
	struct GetResponse *retorno;

	retorno = STAILQ_FIRST(&fila_response_get);
	STAILQ_REMOVE_HEAD(&fila_response_get, entry);

	return retorno;
}

/*! Funcao a ser executada pelas threads trabalhadoras */
void *funcao_thread (void *id)
{
	int indice;
	int tam_buffer;
	int sock_local;
	int bytes_lidos;
	unsigned long frame;
	char buffer[BUFFERSIZE+1];
	char enviar[3] = "ok";
	struct sockaddr_in addr_local;
	sigset_t set;

	signal(SIGINT, signal_handler);
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

	/*! Inicia SOCK_DGRAM : usar o AF_UNIX */
	sock_local = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock_local == -1)
	{
		perror("sock local()");
		return NULL;
	}

	/*! Configura SOCK_DGRAM */
	addr_local.sin_family = AF_INET;
	addr_local.sin_port = htons(PORTA_THREADS);
	addr_local.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(&(addr_local.sin_zero), '\0', 8);

	while (1)
	{
		/*! Condition espera ate que um request seja colocado na lista */
		pthread_mutex_lock(&mutex_master);

		while (STAILQ_EMPTY(&fila_request_get) && STAILQ_EMPTY(&fila_request_put)
						&& (quit == 0))
		{
			pthread_cond_wait(&condition_master, &mutex_master);
		}
		pthread_mutex_unlock(&mutex_master);

		if (quit)
		{
			close(sock_local);
			return NULL;
		}

		/*! GET request */
		pthread_mutex_lock(&mutex_fila_request_get);
		if (STAILQ_EMPTY(&fila_request_get))
		{
			pthread_mutex_unlock(&mutex_fila_request_get);
		}
		else
		{
			get_request *r_aux = retira_fila_request_get();
			indice = r_aux->indice;
			free(r_aux);

			pthread_mutex_unlock(&mutex_fila_request_get);

			/*! Le o arquivo */
			pthread_mutex_lock(&clientes_threads[indice].mutex);
			bytes_lidos = leitura_arquivo(indice);
			pthread_mutex_unlock(&clientes_threads[indice].mutex);

			/*! Erro na leitura */
			if (bytes_lidos < 0)
			{
				quit = 1;
			}
			else
			{
				/*! Envia os bytes lidos */
				if ((bytes_lidos > 0)
							|| ((bytes_lidos == 0)
										&& (clientes_ativos[indice].bytes_enviados
													!= clientes_ativos[indice].bytes_lidos)))
				{
					if (sendto(sock_local, enviar, sizeof(enviar), 0,
							(struct sockaddr *) &addr_local, sizeof(struct sockaddr)) == -1)
					{
						perror("sendto()");
						quit = 1;
					}
				}
				/*! Bytes lidos = 0 */
				else
				{
					encerra_cliente(indice);
				}
			}
		}

		/*! PUT request */
		pthread_mutex_lock(&mutex_fila_request_put);
		if (STAILQ_EMPTY(&fila_request_put))
		{
			pthread_mutex_unlock(&mutex_fila_request_put);
		}
		else
		{
			struct PutRequest *r_aux = retira_fila_request_put();
			indice = r_aux->indice;
			tam_buffer = r_aux->tam_buffer;
			memcpy(buffer, r_aux->buffer, tam_buffer);
			frame = r_aux->frame;
			free(r_aux);
			pthread_mutex_unlock(&mutex_fila_request_put);

			/*! Escreve no arquivo */
			pthread_mutex_lock(&clientes_threads[indice].mutex);

			/*! Controlar a ordem de escrita dos frames recebidos */
			while (frame != clientes_ativos[indice].frame_escrito)
			{
				pthread_cond_wait(&clientes_threads[indice].cond,
													&clientes_threads[indice].mutex);
			}

			bytes_lidos = grava_arquivo(indice, tam_buffer, buffer);

			/*! Erro na escrita */
			if (bytes_lidos <= 0)
			{
				quit = 1;
			}
			else
			{
				if (clientes_ativos[indice].bytes_lidos
							>= clientes_ativos[indice].tam_arquivo)
				{
					encerra_cliente(indice);
				}
			}
			pthread_cond_broadcast(&clientes_threads[indice].cond);
			pthread_mutex_unlock(&clientes_threads[indice].mutex);
		}
	}
	close(sock_local);
	return NULL;
}

put_request *retira_fila_request_put ()
{
	struct PutRequest *retorno;

	retorno = STAILQ_FIRST(&fila_request_put);
	STAILQ_REMOVE_HEAD(&fila_request_put, entry);

	return retorno;
}

/*! Retorna -1: erro na leitura
 * Retorna 0: terminou a leitura do arquivo
 * Retorna > 0: bytes lidos
 */
int leitura_arquivo (int indice)
{
	char *buf = NULL;
  unsigned int bytes_lidos = 0;
	unsigned long pular_buf = 0;
	size_t tam_alocar = 0;

	tam_alocar = calcula_tam_alocar(indice);
	buf = malloc(sizeof(char) * tam_alocar);

	/*! Inicia variaveis locais para melhorar leitura do codigo */
	pular_buf = clientes_ativos[indice].bytes_lidos;

	/*! Nao deixa escrever alem do tamanho do arquivo */
	if (clientes_ativos[indice].tam_arquivo == pular_buf)
	{
		free(buf);
		return 0;
	}

	/*! Reposiciona o ponteiro para ler o arquivo */
	if (fseek(clientes_ativos[indice].fp, pular_buf, SEEK_SET) != 0)
	{
		perror("fseek()");
		free(buf);
		fclose(clientes_ativos[indice].fp);
		return -1;
	}

	/*! Leitura de um buffer do arquivo */
	bytes_lidos = fread(buf, 1, tam_alocar, clientes_ativos[indice].fp);

	if (bytes_lidos > 0)
	{
		clientes_ativos[indice].bytes_lidos += bytes_lidos;
		insere_fila_response_get(buf, indice, bytes_lidos);
	}
	else if (ferror(clientes_ativos[indice].fp))
	{
		return -1; /*! Erro na leitura */
	}
	else
	{
		return 0; /*! EOF */
	}

	free(buf);
	return bytes_lidos;
}

void insere_fila_response_get (char *buf, int indice, int tam_buffer)
{
	pthread_mutex_lock(&mutex_fila_response_get);

	struct GetResponse *paux;

	paux = (struct GetResponse *) malloc(sizeof(struct GetResponse));
	paux->indice = indice;
	paux->tam_buffer = tam_buffer;
	memcpy(paux->buffer, buf, tam_buffer);

	STAILQ_INSERT_TAIL(&fila_response_get, paux, entry);

	pthread_mutex_unlock(&mutex_fila_response_get);
}

/* Retorna 1: request sucesso ('e preciso fwrite ou fread)
 * Retorna 0: request incorreto (retorna para a thread principal a resposta)
 */
void cabecalho_parser (int indice)
{
	int size_strlen;
	char buffer[BUFFERSIZE+1];
	char *http_metodo;
  char *http_versao;
  char *pagina;
  char *contexto;
	int inserir = 0;

	/*! Cria o buffer para nao alterar o cabecalho do cliente */
	memset(buffer, '\0', sizeof(buffer));
	size_strlen = strlen(clientes_ativos[indice].cabecalho);
	strncpy(buffer, clientes_ativos[indice].cabecalho, size_strlen);
  printf("Recebeu mensagem: %s\n", buffer);

  /*! Verifica se tem o GET no inicio */
  http_metodo = strtok_r(buffer, " ", &contexto);

	/*! Recupera a versao do protocolo HTTP : HTTP/1.0 ou HTTP/1.1 */
	pagina = strtok_r(NULL, " ", &contexto);
  http_versao = strtok_r(NULL, "\r", &contexto);

	/*! Se nao houve a versao do protocolo HTTP : request invalido */
	if (pagina == NULL || http_versao == NULL)
	{
		/*! Envia resposta ao cliente, encerra a conexao com ele, return */
		envia_cabecalho(indice, "HTTP/1.0 400 Bad Request\r\n\r\n", 1);
		return;
	}
  else if ((strncmp(http_versao, "HTTP/1.0", 8) != 0)
          && (strncmp(http_versao, "HTTP/1.1", 8) != 0))
  {
		envia_cabecalho(indice, "HTTP/1.0 400 Bad Request\r\n\r\n", 1);
		return;
	}

	/*! Nao permite acesso fora do diretorio especificado */
	recupera_caminho(indice, pagina);
	size_strlen = strlen(diretorio);
  if (strncmp(diretorio, clientes_ativos[indice].caminho,
			size_strlen * sizeof(char)) != 0)
	{
		envia_cabecalho(indice, "HTTP/1.0 403 Forbidden\r\n\r\n", 1);
		return;
	}

	/*! Se houve o GET no inicio */
  if (strncmp(http_metodo, "GET", 3) == 0)
  {
		/*! Verifica se o arquivo pode ser utilizado */
		inserir = arquivo_pode_utilizar(indice, 0);
		if (inserir == -1)
		{
			envia_cabecalho(indice, "HTTP/1.0 503 Service Unavailable\r\n\r\n", 1);
			return;
		}
		else if (inserir == 0)
		{
			/*! Insere na lista de arquivos utilizados */
			insere_lista_arquivos(indice, 0);
		}
		cabecalho_get(indice);
	}
	/*! Se houve o PUT no inicio*/
	else if (strncmp(http_metodo, "PUT", 3) == 0)
	{
		/*! Verifica se o arquivo pode ser utilizado */
		inserir = arquivo_pode_utilizar(indice, 1);
		if (inserir == -1)
		{
			envia_cabecalho(indice, "HTTP/1.0 503 Service Unavailable\r\n\r\n", 1);
			return;
		}
		else if (inserir == 0)
		{
			/*! Insere na lista de arquivos utilizados */
			insere_lista_arquivos(indice, 1);
		}
		cabecalho_put(indice);
	}
  /*! Se nao houver o GET nem PUT no inicio : 501 Not Implemented */
  else
  {
		envia_cabecalho(indice, "HTTP/1.0 501 Not Implemented\r\n\r\n", 1);
		return;
  }
}

/*!
 * Mensagem com o formato da entrada necessaria para a execucao do programa.
 */
void formato_mensagem ()
{
  printf("Formato 1: ./servidor <porta> <diretorio> <velocidade>\n");
	printf("Formato 2: ./servidor <porta> <diretorio>\n");
  printf("Portas validas: 0 a 65535\n");
	printf("Velocidades validas: maiores que 0\n");
}

/*!
 * \brief Cria e configura um socker para o servidor. Conecta o servidor em
 * porta indicada. Escuta conexoes de clientes.
 * \param[in] sock Ponteiro para o socket que identifica o servidor
 * \param[in] servidor Ponteiro para a strcut de configuracao do endereco do
 * servidor
 * \param[in] porta Porta que o servidor deve conectar
 */
void inicia_servidor (int *sock, struct sockaddr_in *servidor, const int porta,
											int *sock_thread, struct sockaddr_in *addr_thread) //struct sockaddr_un addr_thread
{
	//int size;

  /*! Cria socket para o servidor */
  (*sock) = socket(PF_INET, SOCK_STREAM, 0);
  if ((*sock) == -1)
  {
    perror("socket()");
		encerra_servidor();
  }

  /*! Permite reuso da porta : evita 'Address already in use' */
  int yes = 1;
  if (setsockopt((*sock), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
  {
    perror("setsockopt()");
		encerra_servidor();
  }

  /*! Configura o socket: INADDR_ANY (bind recupera IP maquina que roda o
   * processo
   */
  (*servidor).sin_family = AF_INET;
  (*servidor).sin_port = htons(porta);
  (*servidor).sin_addr.s_addr = htonl(INADDR_ANY);
  memset(&((*servidor).sin_zero), '\0', 8);

	/*! Servidor setado para receber conexoes */
  if (bind((*sock), (struct sockaddr *) &(*servidor), sizeof(struct sockaddr))
        == -1)
  {
    perror("bind()");
		encerra_servidor();
  }

  /*! Espera conexoes : maximo de MAXCLIENTS */
  if (listen((*sock), MAXCLIENTS) == -1)
  {
    perror("listen()");
		encerra_servidor();
  }

  /* Socket local para as threads */
	//(*sock_thread) = socket(AF_UNIX, SOCK_DGRAM, 0);
	(*sock_thread) = socket(PF_INET, SOCK_DGRAM, 0);
	if ((*sock_thread) == -1)
	{
		perror("socket thread()");
		encerra_servidor();
	}

	/*! Sock thread nao bloqueante */
	struct timeval read_timeout;
	read_timeout.tv_sec = 0;
	read_timeout.tv_usec = 1;
	setsockopt((*sock_thread), SOL_SOCKET, SO_RCVTIMEO, &read_timeout,
							sizeof read_timeout);

	(*addr_thread).sin_family = AF_INET;
	(*addr_thread).sin_addr.s_addr = htonl(INADDR_ANY);
	(*addr_thread).sin_port = htons(PORTA_THREADS);
	/*(*addr_thread).sun_family = AF_UNIX;
	strncpy((*addr_thread).sun_path, "temp");
	unlink((*addr_thread).sun_path);
	size = strlen((*addr_thread).sub_path) + sizeof((*addr_thread).sun_family);*/
	if (bind((*sock_thread), (struct sockaddr *) &(*addr_thread),
						sizeof(struct sockaddr)) == -1)
	{
		perror("bind thread()");
		encerra_servidor();
	}
}

/*!
 * \brief Atribui valores default para cliente presente no array de clientes
 * ativos. Posicoes zeradas no array sao considerados clientes inativos.
 * \param[in] indice Posicao do array de clientes ativos que sera
 * modificada.
 */
void zera_struct_cliente (int indice)
{
	clientes_ativos[indice] = cliente_vazio;
	clientes_ativos[indice].sock = -1;
}

void encerra_servidor ()
{
	int i;

	for (i = 0; i < MAXCLIENTS; i++)
	{
		pthread_mutex_destroy(&clientes_threads[i].mutex);
		pthread_cond_destroy(&clientes_threads[i].cond);
		if (clientes_ativos[i].sock != -1)
			encerra_cliente(i);
	}

	pthread_mutex_lock(&mutex_master);
	pthread_cond_broadcast(&condition_master);
	pthread_mutex_unlock(&mutex_master);

	/*! Espera as threads acabarem */
	for (i = 0; i < NUM_THREADS; i++)
	{
		pthread_join(threads[i], NULL);
	}

	/*! Encerra todas as threads/mutex/condition */
	pthread_mutex_destroy(&mutex_master);
	pthread_cond_destroy(&condition_master);
	pthread_mutex_destroy(&mutex_fila_request_get);
	pthread_mutex_destroy(&mutex_fila_response_get);
	pthread_mutex_destroy(&mutex_fila_request_put);

	/*! Libera todas as filas */
	struct GetRequest *g1;
	while (!STAILQ_EMPTY(&fila_request_get))
	{
		g1 = STAILQ_FIRST(&fila_request_get);
		STAILQ_REMOVE_HEAD(&fila_request_get, entry);
		free(g1);
	}

	struct GetResponse *g2;
	while (!STAILQ_EMPTY(&fila_response_get))
	{
		g2 = STAILQ_FIRST(&fila_response_get);
		STAILQ_REMOVE_HEAD(&fila_response_get, entry);
		free(g2);
	}

	struct PutRequest *p1;
	while (!STAILQ_EMPTY(&fila_request_put))
	{
		p1 = STAILQ_FIRST(&fila_request_put);
		STAILQ_REMOVE_HEAD(&fila_request_put, entry);
		free(p1);
	}

	pthread_exit(NULL);
	exit(1);
}

/*!
 * \brief Encerra conexao com determinado cliente.
 * \param[in] indice Posicao do array de clientes ativos que determina
 * quem sera processado.
 */
void encerra_cliente (int indice)
{
	if (clientes_ativos[indice].sock != -1)
	{
		remove_arquivo_lista(indice);
		FD_CLR(clientes_ativos[indice].sock, &read_fds);
		printf("Encerrada conexao com socket %d\n",
						clientes_ativos[indice].sock);
		if (clientes_ativos[indice].fp != NULL)
		{
			fclose(clientes_ativos[indice].fp);
		}
		close(clientes_ativos[indice].sock);
		zera_struct_cliente(indice);
		ativos--;
		if (ativos < 0)
			ativos = 0;
	}
}

void remove_arquivo_lista (int indice)
{
	struct Arquivos *paux, *paux2;
	int size_strlen;

	paux = lista_arquivos;
	paux2 = paux;
	size_strlen = strlen(clientes_ativos[indice].caminho);

	while (paux != NULL)
	{
		if ((strncmp(paux->caminho, clientes_ativos[indice].caminho,
									size_strlen) == 0)
					&& (paux->socket == clientes_ativos[indice].sock))
		{
			if (paux == lista_arquivos)
			{
				lista_arquivos = paux->next;
				free(paux);
				return;
			}
			else
			{
				paux2->next = paux->next;
				free(paux);
				return;
			}
		}
		paux2 = paux;
		paux = paux->next;
	}
}

/*!
 * \brief Verifica o tempo que o cliente deve esperar para nao
 * ultrapassar a banda maxima.
 * \param[in] indice Posicao do array de clientes ativos que determina
 * quem sera processado.
 * \param[in] ativos Quantidade de clientes ativos, para otimizar o servidor
 * em caso de um unico cliente.
 */
void verifica_banda(int indice, int ativos)
{
	struct timeval t_atual;

	pthread_mutex_lock(&clientes_threads[indice].mutex);
	if (banda_maxima > 0)
	{
		gettimeofday(&t_atual, NULL);

		/*! Calcula o tempo levado para enviar o buffer ao cliente
		 *  (em microssegundos)
		 */
		struct timeval t_aux;
		struct timeval t_segundo;
		t_segundo.tv_sec = 1;
		t_segundo.tv_usec = 0;

		timersub(&t_atual, &t_janela, &t_aux);

		/*! Buffer enviado em menos de 1 segundo */
		if (timercmp(&t_aux, &t_segundo, <))
		{
			/*! Buffer enviado continha o maximo de bytes possiveis */
			if (clientes_ativos[indice].bytes_por_envio >=
						(unsigned long) banda_maxima)
			{
				/*! Seta o time out para esse cliente */
				timersub(&t_segundo, &t_aux, &timeout);

				/*! Reseta a quantidade de bytes_por_envio do cliente */
				clientes_ativos[indice].bytes_por_envio = 0;
				clientes_ativos[indice].pode_enviar = 0;
			}
			else
			{
				/*! Ainda pode mandar mais bytes, sem estourar a banda maxima */
				if (ativos == 1)
				{
					int temp;
					temp = banda_maxima - clientes_ativos[indice].bytes_por_envio;
					if ((temp > 0) && (temp < (BUFFERSIZE+1)))
					{
						clientes_ativos[indice].pode_enviar = temp;
					}
				}
			}
		}
		/*! Buffer enviado em mais de 1 segundo */
		else
		{
			/*! Reseta a contagem */
			timerclear(&timeout);
			timerclear(&t_janela);
			clientes_ativos[indice].bytes_por_envio = 0;
			clientes_ativos[indice].pode_enviar = 0;
		}
	}
	pthread_mutex_unlock(&clientes_threads[indice].mutex);
}

void insere_fila_request_get (int indice)
{
	pthread_mutex_lock(&mutex_fila_request_get);

	struct GetRequest *paux;

	paux = (struct GetRequest *) malloc(sizeof(struct GetRequest));
	paux->indice = indice;
	STAILQ_INSERT_TAIL(&fila_request_get, paux, entry);


	pthread_mutex_lock(&mutex_master);
	pthread_cond_broadcast(&condition_master);
	pthread_mutex_unlock(&mutex_master);

	pthread_mutex_unlock(&mutex_fila_request_get);
}

void recebe_cabecalho_cliente (int indice)
{
	int bytes = 0;
  int sock_cliente;
	char buffer[BUFFERSIZE+1];

	sock_cliente = clientes_ativos[indice].sock;
	memset(buffer, '\0', sizeof(buffer));

	/*! Recupera o cabecalho */
	int fim_cabecalho = 0;
	while ((!fim_cabecalho)
					&& ((bytes = recv(sock_cliente, buffer, sizeof(buffer), 0)) > 0))
	{
		memcpy(clientes_ativos[indice].cabecalho +
							clientes_ativos[indice].tam_cabecalho, buffer, bytes);
		clientes_ativos[indice].tam_cabecalho += bytes;
		memset(buffer, 0, sizeof(buffer));

		if (strstr(clientes_ativos[indice].cabecalho, "\r\n\r\n")!= NULL)
		{
			fim_cabecalho = 1;
			clientes_ativos[indice].recebeu_cabecalho = 1;
		}
	}
	if (bytes < 0)
	{
		perror("recv()");
		if (errno != EBADF)
		{
			encerra_cliente (indice);
		}
	}
	else if (bytes == 0)
	{
		printf("Socket %d encerrou a conexao.\n", sock_cliente);
		encerra_cliente (indice);
	}
	else
	{
		cabecalho_parser(indice);
	}
}

void recebe_mensagem_cliente (int indice)
{
	if (strncmp(clientes_ativos[indice].cabecalho, "GET", 3) == 0)
	{
		insere_fila_request_get(indice);
	}
	else
	{
		recebe_arquivo_put(indice);
	}
}

int calcula_tam_alocar (int indice)
{
	int tam_alocar = 0;
	if (controle_velocidade)
	{
		if (banda_maxima < (BUFFERSIZE+1))
		{
			tam_alocar = banda_maxima;
		}
		else
		{
			if (clientes_ativos[indice].pode_enviar != 0)
			{
				tam_alocar = clientes_ativos[indice].pode_enviar;
				clientes_ativos[indice].pode_enviar = 0;
			}
			else
			{
				tam_alocar = BUFFERSIZE+1;
			}
		}
	}
	else
	{
		tam_alocar = BUFFERSIZE+1;
	}
	return tam_alocar;
}


void recebe_arquivo_put (int indice)
{
	int bytes;
	char *buf;
	int sock;
	int tam_alocar;

	/*! Se o tempo de envio da iteracao nao estiver setado */
	if (controle_velocidade && timerisset(&t_janela) == 0)
	{
		gettimeofday(&t_janela, NULL);
	}

	tam_alocar = calcula_tam_alocar(indice);

	buf = malloc(sizeof(char) * tam_alocar);
	sock = clientes_ativos[indice].sock;
	bytes = recv(sock, buf, tam_alocar, MSG_DONTWAIT);

	if (bytes > 0)
	{
		clientes_ativos[indice].bytes_enviados += bytes;
		clientes_ativos[indice].bytes_por_envio += bytes;

		insere_fila_request_put(indice, buf, bytes,
														clientes_ativos[indice].frame_recebido);
		clientes_ativos[indice].frame_recebido++;
	}
	else if (bytes < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			perror("recv()");
			encerra_cliente(indice);
		}
	}
	else if (bytes == 0)
	{
		printf("Socket %d encerrou a conexao.\n", sock);
		encerra_cliente (indice);
	}
	free(buf);
}

/*!
 * \brief Recupera o path do arquivo requisitado pelo cliente.
 * \param[in] indice Indice do array de clientes ativos que identica qual
 * cliente esta sendo processado.
 * \param[in] pagina Arquivo requisitado pelo cliente.
 */
void recupera_caminho (int indice, char *pagina)
{
	int tam_pagina = strlen(pagina);
	int tam_diretorio = strlen(diretorio);
	char caminho[PATH_MAX+1];

	memset(caminho, '\0', sizeof(caminho));
	strncpy(caminho, diretorio, tam_diretorio);
	strncat(caminho, pagina, tam_pagina);

	realpath(caminho, clientes_ativos[indice].caminho);
}

/*!
 * \brief Envia o cabecalho da resposta ao cliente.
 * \param[in] indice Indice do array de clientes ativos que identifica
 * qual cliente esta sendo processado.
 * \param[in] cabecalho Cabecalho HTTP a ser enviado.
 * \param[in] flag_erro Flag que indica se o cabecalho contem mensagem de erro
 * HTTP.
 * \return -1 em caso de erro de envio.
 * \return 0 em caso de envio correto.
 */
int envia_cabecalho (int indice, char cabecalho[], int flag_erro)
{
	int size_strlen;
	int temp;
	size_strlen = strlen(cabecalho);
	temp = send(clientes_ativos[indice].sock, cabecalho, size_strlen,
							MSG_NOSIGNAL);

	if (temp == -1)
	{
		perror("send() cabecalho");
		encerra_cliente(indice);
		return -1;
	}
	if (flag_erro)
	{
		encerra_cliente(indice);
	}
	return 0;
}

/*! Verifica se o path indicado pelo cliente 'e um diretorio */
int caminho_diretorio (char *caminho)
{
	struct stat buffer = {0};
	stat(caminho, &buffer);
	return S_ISDIR(buffer.st_mode);
}

void cabecalho_get (int indice)
{
  /*! Casos de request valido: GET /path/to/file HTTP/1.0 (ou HTTP/1.1) */
	if ((!caminho_diretorio(clientes_ativos[indice].caminho))
				&& existe_pagina(clientes_ativos[indice].caminho))
  {
		clientes_ativos[indice].fp = fopen(clientes_ativos[indice].caminho, "rb");

		if (clientes_ativos[indice].fp == NULL)
		{
			perror("fopen()");
			envia_cabecalho(indice, "HTTP/1.0 404 Not Found\r\n\r\n", 1);
		}
		else
		{
			/*! Recupera o tamanho do arquivo e grava na struct clientes_ativos */
			pthread_mutex_lock(&clientes_threads[indice].mutex);
			struct stat st;
			stat(clientes_ativos[indice].caminho, &st);
			clientes_ativos[indice].tam_arquivo = st.st_size;
			pthread_mutex_unlock(&clientes_threads[indice].mutex);

			if (envia_cabecalho(indice, "HTTP/1.0 200 OK\r\n\r\n", 0) == 0)
			{
				insere_fila_request_get(indice);
			}
		}
	}
  else
  {
		envia_cabecalho(indice, "HTTP/1.0 404 Not Found\r\n\r\n", 1);
	}
}

/*!
 * \brief Verifica se um arquivo pode ser utilizado pelo cliente.
 * \param[in] indice Indice do cliente no array clientes_ativos.
 * \param[in] arquivo_is_put Flag indica se o arquivo esta sendo utilizado por
 * um PUT request.
 * \return -1 caso o arquivo nao possa ser utilizado no momento.
 * \return 0 caso o arquivo possa ser utilizado mas nao esta na lista.
 * \return 1 caso o arquivo possa ser utilizado e ja esteja na lista.
 */
int arquivo_pode_utilizar (int indice, int arquivo_is_put)
{
	struct Arquivos *paux;
	int size_strlen;

	if (lista_arquivos == NULL)
	{
		return 0;
	}
	else
	{
		paux = lista_arquivos;
		size_strlen = strlen(clientes_ativos[indice].caminho);
		while (paux != NULL)
		{
			/*! Arquivo presente na lista de arquivos abertos */
			if (strncmp(paux->caminho, clientes_ativos[indice].caminho,
										size_strlen) == 0)
			{
				/*! Verifica se arquivo ja esta sendo acessado por um PUT request */
				if (paux->arquivo_is_put || arquivo_is_put)
				{
					return -1;
				}
				return 1;
			}
			paux = paux->next;
		}
	}
	return 0;
}

void insere_lista_arquivos(int indice, int arquivo_is_put)
{
	struct Arquivos *paux, *pnew;
	int size_strlen;

	size_strlen = strlen(clientes_ativos[indice].caminho);

	pnew = malloc(sizeof(struct Arquivos));
	strncpy(pnew->caminho, clientes_ativos[indice].caminho, size_strlen);
	pnew->socket = clientes_ativos[indice].sock;
	pnew->arquivo_is_put = arquivo_is_put;
	pnew->next = NULL;

	if (lista_arquivos == NULL)
	{
		lista_arquivos = pnew;
	}
	else
	{
		paux = lista_arquivos;
		while (paux->next != NULL)
		{
			paux = paux->next;
		}
		paux->next = pnew;
	}
}

/*! Retorna 0 se nao conseguiu recuperar o tamanho, e 1 se conseguiu*/
int recupera_tam_arquivo (int indice)
{
	char cabecalho[BUFFERSIZE+1];
	char content_length[20] = "Content-Length: ";
	char numero_tamanho[100];
	char *recuperar;
	int tam_cabecalho;
	int tam_content;
	int i;

	tam_cabecalho = strlen(clientes_ativos[indice].cabecalho);
	tam_content = strlen(content_length);
	strncpy(cabecalho, clientes_ativos[indice].cabecalho, tam_cabecalho);

	/*! Encontra a primeira ocorrencia de content-length no cabecalho */
	recuperar = memmem(cabecalho, tam_cabecalho, content_length, tam_content);

	if (recuperar == NULL)
	{
		return 0;
	}

	recuperar += tam_content;

	i = 0;
	while (isdigit(*recuperar))
	{
		numero_tamanho[i] = *recuperar;
		recuperar += 1;
		i++;
	}
	numero_tamanho[i] = '\0';
	long tam_arquivo = atol(numero_tamanho);
	clientes_ativos[indice].tam_arquivo = tam_arquivo;

	return 1;
}

void cabecalho_put (int indice)
{
	FILE *put_result = NULL;
	int temp;

	/*! Recupera o tamanho do arquivo que vai receber (Content-Length) */
	if(!recupera_tam_arquivo(indice))
	{
		envia_cabecalho(indice, "HTTP/1.1 411 Length Required\r\n\r\n", 1);
		return;
	}

	/* Se o diretorio for valido */
	if ((!caminho_diretorio(clientes_ativos[indice].caminho))
				&& (existe_diretorio(clientes_ativos[indice].caminho)))
	{
		/*! Se o arquivo ja existe: retornar 200 OK */
		if (existe_pagina(clientes_ativos[indice].caminho))
		{
			put_result = fopen(clientes_ativos[indice].caminho, "rb");
			if (put_result == NULL)
			{
				perror("fopen()");
				envia_cabecalho(indice, "HTTP/1.0 404 Not Found\r\n\r\n", 1);
				return;
			}
			else
			{
				temp = envia_cabecalho(indice, "HTTP/1.0 200 OK\r\n\r\n", 0);
				if (temp == -1)
				{
					fclose(put_result);
					return;
				}
			}
			fclose(put_result);
		}
		/*! Se o arquivo ainda nao existe: retornar 201 Created*/
		else
		{
			temp = envia_cabecalho(indice, "HTTP/1.0 201 Created\r\n\r\n", 0);
			if (temp == -1)
			{
				return;
			}
		}

		/*! Abre o arquivo */
		clientes_ativos[indice].fp = fopen(clientes_ativos[indice].caminho, "wb");
	}
	/* Se o diretorio nao for valido */
	else
  {
		envia_cabecalho(indice, "HTTP/1.0 404 Not Found\r\n\r\n", 1);
	}
}

void insere_fila_request_put(int indice, char *buf, int tam_buffer,
														 unsigned long frame)
{
	pthread_mutex_lock(&mutex_fila_request_put);

	struct PutRequest *paux;

	paux = (struct PutRequest *) malloc(sizeof(struct PutRequest));
	paux->indice = indice;
	paux->tam_buffer = tam_buffer;
	memcpy(paux->buffer, buf, tam_buffer);
	paux->frame = frame;

	STAILQ_INSERT_TAIL(&fila_request_put, paux, entry);

	pthread_mutex_lock(&mutex_master);
	pthread_cond_broadcast(&condition_master);
	pthread_mutex_unlock(&mutex_master);

	pthread_mutex_unlock(&mutex_fila_request_put);
}

int grava_arquivo (int indice, int tam_buffer, char *buffer)
{
	int bytes_escritos;

	fseek(clientes_ativos[indice].fp, 0L, SEEK_END);

	bytes_escritos = fwrite(buffer, tam_buffer, 1, clientes_ativos[indice].fp);

	if (bytes_escritos > 0)
	{
		clientes_ativos[indice].bytes_lidos += (tam_buffer * bytes_escritos);
		clientes_ativos[indice].bytes_por_envio += (tam_buffer * bytes_escritos);
		clientes_ativos[indice].frame_escrito++;
	}
	return bytes_escritos;
}

int existe_diretorio (char *caminho)
{
	char *caminho_aux;
	char caminho_final[PATH_MAX+1];
	int tamanho;
	int ch = '/';

	memset(caminho_final, '\0', sizeof(caminho_final));
	caminho_aux = strrchr(caminho, ch);
	tamanho = strlen(caminho) - strlen(caminho_aux);
	strncpy(caminho_final, caminho, tamanho);

	struct stat buffer;
	return ((stat(caminho_final, &buffer) == 0) && S_ISDIR(buffer.st_mode));
}

/*!
 * \brief Envia ao cliente uma mensagem. Trata SIGPIPE.
 * \param[in] indice Indice do array de clientes ativos que identica
 * qual cliente esta sendo processado.
 * \param[in] mensagem Mensagem a ser enviada para o cliente.
 * \param[in] size Tamanho da mensagem a ser enviada.
 * \return Quantidade de bytes enviados para o cliente, caso de envio correto.
 * \return -1 em caso de erro no envio.
 */
int envia_cliente (int indice, char mensagem[], int size)
{
	int enviado;

	/*! Se o tempo de envio da iteracao nao estiver setado */
	if (controle_velocidade && timerisset(&t_janela) == 0)
	{
		gettimeofday(&t_janela, NULL);
	}

	enviado = send(clientes_ativos[indice].sock, mensagem, size, MSG_NOSIGNAL);

	if (enviado < 0)
	{
    perror("send()");
		encerra_cliente(indice);
		return -1;
	}

	clientes_ativos[indice].bytes_enviados += enviado;
	clientes_ativos[indice].bytes_por_envio += enviado;

	if (clientes_ativos[indice].bytes_enviados
				>= (unsigned) clientes_ativos[indice].tam_arquivo)
	{
		encerra_cliente(indice);
	}
	return enviado;
}

/*!
 * \brief Valida a existencia do arquivo requisitado pelo usuario.
 * \param[in] caminho Path do arquivo requisitado.
 * \return 1 caso o arquivo exista no caminho determinado.
 * \return 0 caso o arquivo nao exista no caminho determinado.
 */
int existe_pagina (char *caminho)
{
  struct stat buffer;
  return (stat (caminho, &buffer) == 0);
}
