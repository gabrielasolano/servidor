#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

/* Numero maximo de conexoes que o servidor vai esperar */
#define MAXQUEUE 10
#define BUFFERSIZE BUFSIZ
#define MAXCLIENTS 10

typedef struct Monitoramento
{
	int sock;
	int enviar;
	unsigned long bytes_enviados;
	char caminho[BUFFERSIZE+1];
} monitoramento;

int existe_pagina (char *caminho);
void formato_mesagem();
void inicia_servidor(int *sock, struct sockaddr_in *servidor, int porta);
void responde_cliente (int indice_cliente);
void recupera_caminho (int indice_cliente, char *pagina);
int envia_cliente (int sock_cliente, char mensagem[], int size_strlen);
void envia_primeiro_buffer (int indice_cliente);
void envia_cabecalho (int indice_cliente, char cabecalho[]);
void envia_buffer (int indice_cliente);
void zera_struct_cliente (int indice_cliente);

struct Monitoramento clientes_ativos[MAXCLIENTS];
char diretorio[2000];
FILE *fp = NULL;

int main (int argc, char **argv)
{
  struct sockaddr_in servidor;
  struct sockaddr_in cliente;
	struct timeval t_espera;
  socklen_t addrlen;
  fd_set read_fds;
  int sock_servidor;
  int sock_cliente;
  int porta;
  int max_fd;
  int i;
	int ativos = 0;
	int pedido_cliente;
	int excluir_ativos = 0;

  /* Valida quantidade de parametros de entrada */
  if (argc != 3)
  {
    printf("Quantidade incorreta de parametros.\n");
    formato_mesagem();
    return 1;
  }

  /* Recupera e valida a porta */
  porta = atoi(argv[1]);
  if (porta < 0 || porta > 65535)
  {
    printf("Porta invalida!");
    formato_mesagem();
    return 1;
  }

  /* Muda o diretorio do processo para o informado na linha de comando */
  if (chdir(argv[2]) == -1)
  {
    perror("chdir()");
    exit(1);
  }

  /* Recupera diretorio absoluto */
  getcwd(diretorio, sizeof(diretorio));
  if (diretorio == NULL)
  {
    perror("getcwd()");
    exit(1);
  }

  printf("Iniciando servidor na porta: %d e no path: %s\n", porta, diretorio);
  
	/* Configuracao inicial (socket servidor, struct dos clientes ativos,
	 * time out do servidor)
	 */
  inicia_servidor(&sock_servidor, &servidor, porta);
	for (i = 0; i < MAXCLIENTS; i++)
	{
		zera_struct_cliente(i);
	}
	t_espera.tv_sec = 2;
	t_espera.tv_usec = 500000;

  /* Loop principal para aceitar e lidar com conexoes do cliente */
  while (1)
  {
		FD_ZERO(&read_fds);
		FD_SET(sock_servidor, &read_fds);
		max_fd = sock_servidor;

    /* Recebe uma nova conexao */
		if (ativos > 0) /* Se houve alguma ativa, limita o tempo de espera */
    {
      pedido_cliente = select(max_fd + 1, &read_fds, NULL, NULL, &t_espera);
    }
    else
    {
      pedido_cliente = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
    }
		/* Validacao do select */
    if (pedido_cliente == -1)
		{
			perror("select()");
			exit(1);
		}

		/* Adiciona as conexoes ativas no read_fds */
		for (i = 0; i < MAXCLIENTS; i++)
		{
			if (clientes_ativos[i].sock != -1)
				FD_SET(clientes_ativos[i].sock, &read_fds);
			if (clientes_ativos[i].sock > max_fd)
				max_fd = clientes_ativos[i].sock;
		}

		/* Aceita novas conexoes */
		if (FD_ISSET(sock_servidor, &read_fds) && (ativos < MAXCLIENTS)
				&& (pedido_cliente > 0))
		{
			addrlen = sizeof(cliente);
			sock_cliente = accept(sock_servidor, (struct sockaddr *)&cliente, &addrlen);
			if (sock_cliente == -1)
			{
				perror("accept()");
			}
			else
			{
				/* Configuracao inicial de um cliente ativo */
				clientes_ativos[ativos].sock = sock_cliente;
				clientes_ativos[ativos].enviar = 1;
				clientes_ativos[ativos].bytes_enviados = 0;

				/* Controle da quantidade de clientes ativos */
				ativos++;

				/* Adiciona o novo socket na read_fds */
				FD_SET(sock_cliente, &read_fds);
				if (sock_cliente > max_fd)
				{
					max_fd = sock_cliente;
				}
				printf("Nova conexao de %s no socket %d\n",
							 inet_ntoa(cliente.sin_addr), (sock_cliente));
			}
		}

		/* Trata as conexoes ativas */
		excluir_ativos = 0;
		for (i = 0; i < MAXQUEUE; i++)
		{
			/* Verifica se esta esperando para leitura */
			if (FD_ISSET(clientes_ativos[i].sock, &read_fds))
			{
				/* Se o cliente ja terminou sua leitura */
				if (clientes_ativos[i].enviar == 0)
				{
					FD_CLR(clientes_ativos[i].sock, &read_fds);
					printf("Encerrada conexao com socket %d\n", clientes_ativos[i].sock);
					close(clientes_ativos[i].sock);
					zera_struct_cliente(i);
					excluir_ativos++;
				}
				/* Se o cliente ainda nao terminou a leitura */
				else
				{
					responde_cliente(i);
				}
			}
		}
		ativos -= excluir_ativos;
  }
  close(sock_servidor);
  return 0;
}

void zera_struct_cliente (int indice_cliente)
{
	clientes_ativos[indice_cliente].sock = -1;
	clientes_ativos[indice_cliente].enviar = 0;
	clientes_ativos[indice_cliente].bytes_enviados = 0;
	memset(clientes_ativos[indice_cliente].caminho, '\0', BUFFERSIZE+1);
}

void inicia_servidor (int *sock, struct sockaddr_in *servidor, const int porta)
{
  /* Cria socket para o servidor */
  (*sock) = socket(PF_INET, SOCK_STREAM, 0);
  if ((*sock) == -1)
  {
    perror("socket()");
    exit(1);
  }

  /* Permite reuso da porta : evita 'Address already in use' */
  int yes = 1;
  if (setsockopt((*sock), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
  {
    perror("setsockopt()");
    exit(1);
  }

  /* Configura o socket: INADDR_ANY (bind recupera IP maquina que roda o
   * processo
   */
  (*servidor).sin_family = AF_INET;
  (*servidor).sin_port = htons(porta);
  (*servidor).sin_addr.s_addr = htonl(INADDR_ANY);
  memset(&((*servidor).sin_zero), '\0', 8);

	/* Conecta */
  if (bind((*sock), (struct sockaddr *) &(*servidor), sizeof(struct sockaddr))
        == -1)
  {
    perror("bind()");
    exit(1);
  }

  /* Espera conexoes : maximo de MAXQUEUE */
  if (listen((*sock), MAXQUEUE) == -1)
  {
    perror("listen()");
    exit(1);
  }
}

 /* Recebe e responde os requests dos clientes */
void responde_cliente (int indice_cliente)
{
	if (clientes_ativos[indice_cliente].bytes_enviados == 0)
	{
		/*Envia cabecalho + 1 buffer de arquivo */
		envia_primeiro_buffer(indice_cliente);
	}
	else
	{
		envia_buffer(indice_cliente);
	}
}

void envia_primeiro_buffer (int indice_cliente)
{
	int bytes;
  int sock_cliente;
	char buffer[BUFFERSIZE+1];
  char *http_metodo;
  char *http_versao;
  char *pagina;
  char *contexto;

	sock_cliente = clientes_ativos[indice_cliente].sock;
	memset(buffer, '\0', sizeof(buffer));
	sleep(5);
  if ((bytes = recv(sock_cliente, buffer, sizeof(buffer), 0)) > 0)
  {
    printf("Recebeu mensagem: %s\n", buffer);

    /* Verifica se tem o GET no inicio */
    http_metodo = strtok_r(buffer, " ", &contexto);

		/* Se houve o GET no inicio :  */
    if (strncmp(http_metodo, "GET", 3) == 0)
    {
      /* Recupera a versao do protocolo HTTP : HTTP/1.0 ou HTTP/1.1 */
      pagina = strtok_r(NULL, " ", &contexto);
      http_versao = strtok_r(NULL, "\r", &contexto);

			/* Se nao houve a versao do protocolo HTTP : request invalido */
      if ((strncmp(http_versao, "HTTP/1.0", 8) != 0)
            && (strncmp(http_versao, "HTTP/1.1", 8) != 0))
      {
				envia_cabecalho(indice_cliente, "HTTP/1.0 400 Bad Request\r\n\r\n");
				clientes_ativos[indice_cliente].enviar = 0;
      }
      /* Casos de request valido: GET /path/to/file HTTP/1.0 (ou HTTP/1.1) */
      else
      {
        /* Nao permite acesso fora do diretorio especificado
         * Exemplo : localhost/../../../dado 
         */
        if ((pagina[0] != '/') || (strncmp(pagina, "/..", 3) == 0))
        {
					envia_cabecalho(indice_cliente, "HTTP/1.0 403 Forbidden\r\n\r\n");
					clientes_ativos[indice_cliente].enviar = 0;
        }
        else
        {
          recupera_caminho(indice_cliente, pagina);
          if (existe_pagina(clientes_ativos[indice_cliente].caminho))
          {
						fp = fopen(clientes_ativos[indice_cliente].caminho, "rb");
						if (fp == NULL)
						{
							perror("fopen()");
							envia_cabecalho(indice_cliente, 
																"HTTP/1.0 404 Not Found\r\n\r\n");
							clientes_ativos[indice_cliente].enviar = 0;
						}
						else
						{
							fclose(fp);
							envia_cabecalho(indice_cliente, "HTTP/1.0 200 OK\r\n\r\n");
							envia_buffer(indice_cliente);
						}
          }
          else
          {
						envia_cabecalho(indice_cliente, "HTTP/1.0 404 Not Found\r\n\r\n");
						clientes_ativos[indice_cliente].enviar = 0;
          }
        }
      }
    }
    /* Se nao houver o GET no inicio : 501 Not Implemented */
    else
    {
			envia_cabecalho(indice_cliente, "HTTP/1.0 501 Not Implemented\r\n\r\n");
			clientes_ativos[indice_cliente].enviar = 0;
    }
  }
  else if (bytes < 0)
  {
    perror("recv()");
  }
  else if (bytes == 0)
  {
    printf("Socket %d encerrou a conexao.\n", sock_cliente);
		zera_struct_cliente(indice_cliente);
  }
}

int envia_cliente (int sock_cliente, char mensagem[], int size_strlen)
{
	int enviado;
	enviado = send(sock_cliente, mensagem, size_strlen, 0);
  if (enviado == -1)
	{
    perror("send()");
		exit(1);
	}
	return enviado;
}

void recupera_caminho (int indice, char *pagina)
{
	int tam_pagina = strlen(pagina);
	int tam_diretorio = strlen(diretorio);

	strncpy(clientes_ativos[indice].caminho, diretorio, tam_diretorio);
	strncat(clientes_ativos[indice].caminho, pagina, tam_pagina);
}

int existe_pagina (char *caminho)
{
  if (access(caminho, R_OK) == 0)
  {
    return 1; /* Arquivo existe com permissao para sua leitura */
  }
  return 0;
}

void formato_mesagem ()
{
  printf("Formato: ./recuperador <porta> <diretorio>\n");
  printf("Portas validas: 0 a 65535\n");
}

void envia_cabecalho (int indice_cliente, char cabecalho[])
{
	int size_strlen;
	size_strlen = strlen(cabecalho);
	envia_cliente(clientes_ativos[indice_cliente].sock, cabecalho, size_strlen);
}

void envia_buffer (int indice_cliente)
{
  char buf[BUFFERSIZE+1];
  unsigned int bytes_lidos = 0;
  int sock_cliente;
	unsigned long pular_buf = 0;

	/* Zera o buffer que vai receber o arquivo */
	memset(buf, '\0', sizeof(buf));

	/* Inicia variaveis locais para melhorar leitura do codigo */
	sock_cliente = clientes_ativos[indice_cliente].sock;
	pular_buf = clientes_ativos[indice_cliente].bytes_enviados;

	/* Abre o arquivo */
	fp = fopen(clientes_ativos[indice_cliente].caminho, "rb");

	/* Reposiciona o ponteiro para ler o arquivo */
	if (fseek(fp, pular_buf, SEEK_SET) != 0)
	{
		perror("fseek()");
		exit(1);
	}

	/* Leitura de um buffer do arquivo */
	bytes_lidos = fread(buf, 1, sizeof(buf), fp);

	/* O que leu foi menor que o tamanho do buffer, logo o arquivo acabou */
	if ((bytes_lidos > 0) && (bytes_lidos < sizeof(buf)))
	{
		envia_cliente(sock_cliente, buf, bytes_lidos);
		clientes_ativos[indice_cliente].enviar = 0;
	}
	else if (bytes_lidos == sizeof(buf))
	{
		/* Leu um buffer inteiro, pode ser que exista mais coisa no arquivo */
		int temp = envia_cliente(sock_cliente, buf, bytes_lidos);
		clientes_ativos[indice_cliente].bytes_enviados += temp;
	}
	else
	{
		/* Nao leu nada, arquivo acabou */
		clientes_ativos[indice_cliente].enviar = 0;
	}
	memset(buf, '\0', sizeof(buf));
	fclose(fp);
}
