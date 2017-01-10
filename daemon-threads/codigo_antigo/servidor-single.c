/*!
 * \file servidor.c
 * \brief Servidor multi-thread interoperando com clientes ativos, com
 * controle de velocidade.
 * \date 16/12/2016
 * \author Gabriela Solano <gabriela.solano@aker.com.br>
 *
 */
#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>

#define BUFFERSIZE BUFSIZ
#define MAXCLIENTS 200

/*!
 * Struct para controle dos clientes ativos.
 */
typedef struct Monitoramento
{
	int sock;
	int enviar;
	int recebeu_cabecalho;
	int tam_cabecalho;
	long tam_arquivo;
	unsigned long bytes_por_envio;
	unsigned long bytes_enviados;
	unsigned long pode_enviar; /*! Bytes para enviar sem estourar a banda */
	char caminho[PATH_MAX+1];
	char cabecalho[BUFFERSIZE+1];
} monitoramento;

typedef struct Arquivos
{
	char caminho[PATH_MAX+1];
	int arquivo_is_put;
	int socket;
	struct Arquivos *next;
} arquivos;

void formato_mensagem();
void inicia_servidor(int *sock, struct sockaddr_in *servidor, int porta);
void zera_struct_cliente (int indice_cliente);
void encerra_cliente (int indice_cliente);
void responde_cliente (int indice_cliente);
void verifica_banda (int indice_cliente, int ativos);
void cabecalho_put (int indice_cliente);
int existe_diretorio (char *caminho);
void grava_arquivo (FILE *put_result, int indice_cliente);
void envia_buffer_put (int indice_cliente);
void cabecalho_get (int indice_cliente);
void envia_buffer_get (int indice_cliente);
void envia_primeiro_buffer (int indice_cliente);
void envia_buffer (int indice_cliente);
void recupera_caminho (int indice_cliente, char *pagina);
int envia_cabecalho (int indice_cliente, char cabecalho[], int flag_erro);
int existe_pagina (char *caminho);
int envia_cliente (int sock_cliente, char mensagem[], int size_strlen);
int arquivo_pode_utilizar (int indice_cliente, int arquivo_is_put);
void insere_lista_arquivos(int indice_cliente, int arquivo_is_put);
void remove_arquivo_lista (int indice_cliente);
void recupera_tam_arquivo (int indice_cliente);

struct Monitoramento clientes_ativos[MAXCLIENTS];
struct Monitoramento cliente_vazio = {0};
struct Arquivos *lista_arquivos = NULL;
char diretorio[PATH_MAX+1];
int excluir_ativos = 0;
int controle_velocidade;
long banda_maxima;
fd_set read_fds;
FILE *fp = NULL;
struct timeval t_janela;
struct timeval timeout;

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
  struct sockaddr_in servidor;
  struct sockaddr_in cliente;
  socklen_t addrlen;
  int sock_servidor;
  int sock_cliente;
  int porta;
  int max_fd;
  int i;
	int ativos = 0;
	int pedido_cliente;
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

	/*! Configuracao inicial (socket servidor, struct dos clientes ativos,
	 * time out do servidor)
	 */
  inicia_servidor(&sock_servidor, &servidor, porta);
	for (i = 0; i < MAXCLIENTS; i++)
	{
		zera_struct_cliente(i);
	}

	timerclear(&t_janela);
	timerclear(&timeout);
	max_fd = sock_servidor;

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
    if (pedido_cliente < 0)
		{
			perror("select()");
			exit(1);
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
				/*! Configuracao inicial de um cliente ativo */
				clientes_ativos[ativos].sock = sock_cliente;
				clientes_ativos[ativos].enviar = 1;

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

		/*! Trata as conexoes ativas */
		excluir_ativos = 0;
		timerclear(&t_janela);
		timerclear(&timeout);
		for (i = 0; i < MAXCLIENTS; i++)
		{
			if (clientes_ativos[i].sock != -1)
			{
				/*! Verifica se esta esperando para leitura */
				if (FD_ISSET(clientes_ativos[i].sock, &read_fds))
				{
					/*! Se o cliente ja terminou sua leitura */
					if (clientes_ativos[i].enviar == 0)
					{
						encerra_cliente(i);
					}
					/*! Se o cliente ainda nao terminou a leitura */
					else
					{
						responde_cliente(i);
						if ((clientes_ativos[i].sock != -1)
									&& (clientes_ativos[i].enviar == 0))
						{
							encerra_cliente(i);
						}
						if (controle_velocidade && clientes_ativos[i].sock != -1)
							verifica_banda(i, ativos);
					}
				}
			}
		}
		ativos -= excluir_ativos;
  }
  close(sock_servidor);
  return 0;
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
void inicia_servidor (int *sock, struct sockaddr_in *servidor, const int porta)
{
  /*! Cria socket para o servidor */
  (*sock) = socket(PF_INET, SOCK_STREAM, 0);
  if ((*sock) == -1)
  {
    perror("socket()");
    exit(1);
  }

  /*! Permite reuso da porta : evita 'Address already in use' */
  int yes = 1;
  if (setsockopt((*sock), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
  {
    perror("setsockopt()");
    exit(1);
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
    exit(1);
  }

  /*! Espera conexoes : maximo de MAXCLIENTS */
  if (listen((*sock), MAXCLIENTS) == -1)
  {
    perror("listen()");
    exit(1);
  }
}

/*!
 * \brief Atribui valores default para cliente presente no array de clientes
 * ativos. Posicoes zeradas no array sao considerados clientes inativos.
 * \param[in] indice_cliente Posicao do array de clientes ativos que sera
 * modificada.
 */
void zera_struct_cliente (int indice_cliente)
{
	clientes_ativos[indice_cliente] = cliente_vazio;
	clientes_ativos[indice_cliente].sock = -1;
	/*clientes_ativos[indice_cliente].enviar = 0;
	clientes_ativos[indice_cliente].bytes_enviados = 0;
	clientes_ativos[indice_cliente].bytes_por_envio = 0;
	clientes_ativos[indice_cliente].pode_enviar = 0;
	clientes_ativos[indice_cliente].recebeu_cabecalho = 0;
	clientes_ativos[indice_cliente].tam_cabecalho = 0;
	clientes_ativos[indice_cliente].tam_arquivo = 0;
	memset(clientes_ativos[indice_cliente].caminho, '\0', BUFFERSIZE+1);
	memset(clientes_ativos[indice_cliente].cabecalho, '\0', BUFFERSIZE+1);*/
}

/*!
 * \brief Encerra conexao com determinado cliente.
 * \param[in] indice_cliente Posicao do array de clientes ativos que determina
 * quem sera processado.
 */
void encerra_cliente (int indice_cliente)
{
	remove_arquivo_lista(indice_cliente);
	FD_CLR(clientes_ativos[indice_cliente].sock, &read_fds);
	printf("Encerrada conexao com socket %d\n",
					clientes_ativos[indice_cliente].sock);
	close(clientes_ativos[indice_cliente].sock);
	zera_struct_cliente(indice_cliente);
	excluir_ativos++;
}

void remove_arquivo_lista (int indice_cliente)
{
	struct Arquivos *paux, *paux2;
	int size_strlen;

	paux = lista_arquivos;
	paux2 = paux;
	size_strlen = strlen(clientes_ativos[indice_cliente].caminho);

	while (paux != NULL)
	{
		if ((strncmp(paux->caminho, clientes_ativos[indice_cliente].caminho, size_strlen) == 0)
				&& (paux->socket == clientes_ativos[indice_cliente].sock))
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
 * \brief Responde requisicao de determinado cliente.
 * \param[in] indice_cliente Indice do array de clientes ativos que determina
 * qual cliente deve ser respondido.
 */
void responde_cliente (int indice_cliente)
{
	if (clientes_ativos[indice_cliente].recebeu_cabecalho == 0)
	{
		/*!Envia cabecalho + 1 buffer de arquivo */
		envia_primeiro_buffer(indice_cliente);
	}
	else
	{
		envia_buffer(indice_cliente);
	}
}

/*!
 * \brief Verifica o tempo que o cliente deve esperar para nao
 * ultrapassar a banda maxima.
 * \param[in] indice_cliente Posicao do array de clientes ativos que determina
 * quem sera processado.
 * \param[in] ativos Quantidade de clientes ativos, para otimizar o servidor
 * em caso de um unico cliente.
 */
void verifica_banda(int indice_cliente, int ativos)
{
	struct timeval t_atual;

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
			if (clientes_ativos[indice_cliente].bytes_por_envio >=
						(unsigned long) banda_maxima)
			{
				/*! Seta o time out para esse cliente */
				timersub(&t_segundo, &t_aux, &timeout);

				/*! Reseta a quantidade de bytes_por_envio do cliente */
				clientes_ativos[indice_cliente].bytes_por_envio = 0;
				clientes_ativos[indice_cliente].pode_enviar = 0;
			}
			else
			{
				/*! Ainda pode mandar mais bytes, sem estourar a banda maxima */
				if (ativos == 1)
				{
					int temp;
					temp = banda_maxima - clientes_ativos[indice_cliente].bytes_por_envio;
					if ((temp > 0) && (temp < (BUFFERSIZE+1)))
					{
						clientes_ativos[indice_cliente].pode_enviar = temp;
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
			clientes_ativos[indice_cliente].bytes_por_envio = 0;
			clientes_ativos[indice_cliente].pode_enviar = 0;
		}
	}
}

void envia_primeiro_buffer (int indice_cliente)
{
	int bytes = 0;
	int inserir = 0;
  int sock_cliente;
	int size_strlen;
	char buffer[BUFFERSIZE+1];
  char *http_metodo;
  char *http_versao;
  char *pagina;
  char *contexto;

	sock_cliente = clientes_ativos[indice_cliente].sock;
	memset(buffer, '\0', sizeof(buffer));

	/* Recupera o cabecalho e grava em 'buffer' */
	int fim_cabecalho = 0;
	while ((!fim_cabecalho)
					&& ((bytes = recv(sock_cliente, buffer, sizeof(buffer), 0)) > 0))
	{
		memcpy(clientes_ativos[indice_cliente].cabecalho +
							clientes_ativos[indice_cliente].tam_cabecalho, buffer, bytes);
		clientes_ativos[indice_cliente].tam_cabecalho += bytes;
		memset(buffer, 0, sizeof(buffer));

		if (strstr(clientes_ativos[indice_cliente].cabecalho, "\r\n\r\n")!= NULL)
		{
			fim_cabecalho = 1;
			clientes_ativos[indice_cliente].recebeu_cabecalho = 1;
		}
	}

	if (bytes < 0)
	{
		perror("recv()");
		if (errno != EBADF)
		{
			encerra_cliente (indice_cliente);
		}
	}
	else if (bytes == 0)
	{
		printf("Socket %d encerrou a conexao.\n", sock_cliente);
		encerra_cliente (indice_cliente);
	}
	else
  {
		size_strlen = strlen(clientes_ativos[indice_cliente].cabecalho);
		strncpy(buffer, clientes_ativos[indice_cliente].cabecalho, size_strlen);
    printf("Recebeu mensagem: %s\n", buffer);

    /*! Verifica se tem o GET no inicio */
    http_metodo = strtok_r(buffer, " ", &contexto);

		/*! Recupera a versao do protocolo HTTP : HTTP/1.0 ou HTTP/1.1 */
		pagina = strtok_r(NULL, " ", &contexto);
    http_versao = strtok_r(NULL, "\r", &contexto);

		/*! Se nao houve a versao do protocolo HTTP : request invalido */
		if (pagina == NULL || http_versao == NULL)
		{
			envia_cabecalho(indice_cliente, "HTTP/1.0 400 Bad Request\r\n\r\n", 1);
			return;
		}
    else if ((strncmp(http_versao, "HTTP/1.0", 8) != 0)
           && (strncmp(http_versao, "HTTP/1.1", 8) != 0))
    {
			envia_cabecalho(indice_cliente, "HTTP/1.0 400 Bad Request\r\n\r\n", 1);
			return;
		}

		/*! Nao permite acesso fora do diretorio especificado */
		recupera_caminho(indice_cliente, pagina);
		size_strlen = strlen(diretorio);
    if (strncmp(diretorio, clientes_ativos[indice_cliente].caminho,
				size_strlen * sizeof(char)) != 0)
		{
			envia_cabecalho(indice_cliente, "HTTP/1.0 403 Forbidden\r\n\r\n", 1);
			return;
		}

		/*! Se houve o GET no inicio :  */
    if (strncmp(http_metodo, "GET", 3) == 0)
    {
			/*! Verifica se o arquivo pode ser utilizado */
			inserir = arquivo_pode_utilizar(indice_cliente, 0);
			if (inserir == -1)
			{
				envia_cabecalho(indice_cliente,
												"HTTP/1.0 503 Service Unavailable\r\n\r\n", 1);
				return;
			}
			else if (inserir == 0)
			{
				/*! Insere na lista de arquivos utilizados */
				insere_lista_arquivos(indice_cliente, 0);
			}
			cabecalho_get(indice_cliente);
		}
		/*! Se houve o PUT no inicio :  */
		else if (strncmp(http_metodo, "PUT", 3) == 0)
		{
			/*! Verifica se o arquivo pode ser utilizado */
			inserir = arquivo_pode_utilizar(indice_cliente, 1);
			if (inserir == -1)
			{
				envia_cabecalho(indice_cliente,
												"HTTP/1.0 503 Service Unavailable\r\n\r\n", 1);
				return;
			}
			else if (inserir == 0)
			{
				/*! Insere na lista de arquivos utilizados */
				insere_lista_arquivos(indice_cliente, 1);
			}
			cabecalho_put(indice_cliente);
		}
    /*! Se nao houver o GET nem PUT no inicio : 501 Not Implemented */
    else
    {
			envia_cabecalho(indice_cliente,
											"HTTP/1.0 501 Not Implemented\r\n\r\n", 1);
    }
  }
}

void envia_buffer (int indice_cliente)
{
	if (strncmp(clientes_ativos[indice_cliente].cabecalho, "GET", 3) == 0)
	{
		envia_buffer_get(indice_cliente);
	}
	else
	{
		envia_buffer_put(indice_cliente);
	}
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
 * \param[in] indice_cliente Indice do array de clientes ativos que identifica
 * qual cliente esta sendo processado.
 * \param[in] cabecalho Cabecalho HTTP a ser enviado.
 * \param[in] flag_erro Flag que indica se o cabecalho contem mensagem de erro
 * HTTP.
 * \return -1 em caso de erro de envio.
 * \return 0 em caso de envio correto.
 */
int envia_cabecalho (int indice_cliente, char cabecalho[], int flag_erro)
{
	int size_strlen;
	int temp;
	size_strlen = strlen(cabecalho);
	temp = envia_cliente(indice_cliente, cabecalho, size_strlen);
	if (temp == -1)
	{
		return -1;
	}
	if (flag_erro)
	{
		clientes_ativos[indice_cliente].enviar = 0;
	}
	return 0;
}

void cabecalho_get (int indice_cliente)
{
  /*! Casos de request valido: GET /path/to/file HTTP/1.0 (ou HTTP/1.1) */
	if (existe_pagina(clientes_ativos[indice_cliente].caminho))
  {
		fp = fopen(clientes_ativos[indice_cliente].caminho, "rb");
		if (fp == NULL)
		{
			perror("fopen()");
			envia_cabecalho(indice_cliente,
											"HTTP/1.0 404 Not Found\r\n\r\n", 1);
		}
		else
		{
			fclose(fp);
			if (envia_cabecalho(indice_cliente, "HTTP/1.0 200 OK\r\n\r\n", 0)
						== 0)
			{
				envia_buffer_get(indice_cliente);
			}
		}
	}
  else
  {
		envia_cabecalho(indice_cliente,
										"HTTP/1.0 404 Not Found\r\n\r\n", 1);
	}
}

int arquivo_pode_utilizar (int indice_cliente, int arquivo_is_put)
{
	/* Verifica se o arquivo ja esta sendo utilizado.
	 *
	 * put + put / put + get / get + put / get + get
	 * nao			/	nao				/	nao				/ ok
	 * Retorna -1: nao pode utilizar
	 * Retorna 0: pode utilizar (tem que adicionar na lista)
	 * Retorna 1: pode utilizar (ja esta na lista)
	 */

	struct Arquivos *paux;
	int size_strlen;

	if (lista_arquivos == NULL)
	{
		return 0;
	}
	else
	{
		paux = lista_arquivos;
		size_strlen = strlen(clientes_ativos[indice_cliente].caminho);
		while (paux != NULL)
		{
			/*! Arquivo presente na lista */
			if (strncmp(paux->caminho, clientes_ativos[indice_cliente].caminho,
										size_strlen) == 0)
			{
				/*! Verifica se arquivo pode ser acessado */
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

void insere_lista_arquivos(int indice_cliente, int arquivo_is_put)
{
	struct Arquivos *paux, *pnew;
	int size_strlen;

	size_strlen = strlen(clientes_ativos[indice_cliente].caminho);

	pnew = malloc(sizeof(struct Arquivos));
	strncpy(pnew->caminho, clientes_ativos[indice_cliente].caminho, size_strlen);
	pnew->socket = clientes_ativos[indice_cliente].sock;
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

/*!
 * \brief Envia um buffer do arquivo requisitado para o cliente. Atualiza o
 * estado do cliente no array de clientes ativos.
 * \param[in] indice_cliente Indice do array de clientes ativos que identifica
 * qual cliente esta sendo processado.
 */
void envia_buffer_get (int indice_cliente)
{
	char *buf = NULL;
  unsigned int bytes_lidos = 0;
	unsigned long pular_buf = 0;
	int temp = 0;
	size_t tam_alocar = 0;

	if (controle_velocidade)
	{
		if (banda_maxima < (BUFFERSIZE+1))
		{
			tam_alocar = banda_maxima;
		}
		else
		{
			if (clientes_ativos[indice_cliente].pode_enviar != 0)
			{
				tam_alocar = clientes_ativos[indice_cliente].pode_enviar;
				clientes_ativos[indice_cliente].pode_enviar = 0;
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
	buf = malloc(sizeof(char) * tam_alocar);

	/*! Inicia variaveis locais para melhorar leitura do codigo */
	pular_buf = clientes_ativos[indice_cliente].bytes_enviados;

	/*! Abre o arquivo */
	fp = fopen(clientes_ativos[indice_cliente].caminho, "rb");

	/*! Reposiciona o ponteiro para ler o arquivo */
	if (fseek(fp, pular_buf, SEEK_SET) != 0)
	{
		perror("fseek()");
		free(buf);
		exit(1);
	}

	/*! Leitura de um buffer do arquivo */
	bytes_lidos = fread(buf, 1, tam_alocar, fp);

	if (bytes_lidos > 0)
	{
		temp = envia_cliente(indice_cliente, buf, bytes_lidos);
		if (temp == -1)
		{
			fclose(fp);
			free(buf);
			return;
		}
		clientes_ativos[indice_cliente].bytes_enviados += temp;
		clientes_ativos[indice_cliente].bytes_por_envio += temp;
	}
	else
	{
		clientes_ativos[indice_cliente].enviar = 0;
	}

	free(buf);
	fclose(fp);
}

void recupera_tam_arquivo (int indice_cliente)
{
	char cabecalho[BUFFERSIZE+1];
	char content_length[20] = "Content-Length: ";
	char numero_tamanho[100];
	char *recuperar;
	int tam_cabecalho;
	int tam_content;
	int i;

	tam_cabecalho = strlen(clientes_ativos[indice_cliente].cabecalho);
	tam_content = strlen(content_length);
	strncpy(cabecalho, clientes_ativos[indice_cliente].cabecalho, tam_cabecalho);

	recuperar = memmem(cabecalho, tam_cabecalho, content_length, tam_content);
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
	clientes_ativos[indice_cliente].tam_arquivo = tam_arquivo;
}

void cabecalho_put (int indice_cliente)
{
	FILE *put_result = NULL;
	int temp;

	/* Se o diretorio for valido */
	if (existe_diretorio(clientes_ativos[indice_cliente].caminho))
	{
		/*! Se o arquivo ja existe: retornar 200 OK */
		if (existe_pagina(clientes_ativos[indice_cliente].caminho))
		{
			temp = envia_cabecalho(indice_cliente, "HTTP/1.0 200 OK\r\n\r\n", 0);
			if (temp == -1)
			{
				return;
			}
		}
		/*! Se o arquivo ainda nao existe: retornar 201 Created*/
		else
		{
			temp = envia_cabecalho(indice_cliente, "HTTP/1.0 201 Created\r\n\r\n", 0);
			if (temp == -1)
			{
				return;
			}
		}

		/*! Recupera o tamanho do arquivo que vai receber (Content-Length) */
		recupera_tam_arquivo(indice_cliente);

		put_result = fopen(clientes_ativos[indice_cliente].caminho, "wb");
		if (put_result == NULL)
		{
			perror("fopen()");
			envia_cabecalho(indice_cliente,
											"HTTP/1.0 404 Not Found\r\n\r\n", 1);
		}
		else
		{
			/*! Grava 1 buffer de arquivo */
			grava_arquivo(put_result, indice_cliente);
		}
		fclose(put_result);
	}
	/* Se o diretorio nao for valido */
	else
  {
		envia_cabecalho(indice_cliente,
										"HTTP/1.0 404 Not Found\r\n\r\n", 1);
	}
}

void envia_buffer_put (int indice_cliente)
{
	FILE *put_result;

	put_result = fopen(clientes_ativos[indice_cliente].caminho, "ab");
	grava_arquivo(put_result, indice_cliente);
	fclose(put_result);
}

void grava_arquivo (FILE *put_result, int indice_cliente)
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

	if (controle_velocidade)
	{
		if (banda_maxima < (BUFFERSIZE+1))
		{
			tam_alocar = banda_maxima;
		}
		else
		{
			if (clientes_ativos[indice_cliente].pode_enviar != 0)
			{
				tam_alocar = clientes_ativos[indice_cliente].pode_enviar;
				clientes_ativos[indice_cliente].pode_enviar = 0;
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

	buf = malloc(sizeof(char) * tam_alocar);
	sock = clientes_ativos[indice_cliente].sock;
	bytes = recv(sock, buf, tam_alocar, MSG_DONTWAIT);

	if (bytes > 0)
	{
		fwrite(buf, bytes, 1, put_result);
		clientes_ativos[indice_cliente].bytes_enviados += bytes;
		clientes_ativos[indice_cliente].bytes_por_envio += bytes;

		if (clientes_ativos[indice_cliente].bytes_enviados
					>= (unsigned) clientes_ativos[indice_cliente].tam_arquivo)
		{
			clientes_ativos[indice_cliente].enviar = 0;
		}
	}
	else if (bytes < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			perror("recv()");
			exit(1);
		}
		free(buf);
		return;
	}
	else if (bytes == 0)
	{
		printf("Socket %d encerrou a conexao.\n", sock);
		encerra_cliente (indice_cliente);
	}
	free(buf);
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
 * \param[in] indice_cliente Indice do array de clientes ativos que identica
 * qual cliente esta sendo processado.
 * \param[in] mensagem Mensagem a ser enviada para o cliente.
 * \param[in] size_strlen Tamanho da mensagem a ser enviada.
 * \return Quantidade de bytes enviados para o cliente, caso de envio correto.
 * \return -1 em caso de erro no envio.
 */
int envia_cliente (int indice_cliente, char mensagem[], int size_strlen)
{
	int enviado;

	/*! Se o tempo de envio da iteracao nao estiver setado */
	if (controle_velocidade && timerisset(&t_janela) == 0)
	{
		gettimeofday(&t_janela, NULL);
	}

	enviado = send(clientes_ativos[indice_cliente].sock, mensagem, size_strlen,
								 MSG_NOSIGNAL);
	if (enviado <= 0)
	{
    perror("send()");
		encerra_cliente (indice_cliente);
		return -1;
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
  struct stat   buffer;
  return (stat (caminho, &buffer) == 0);
}
