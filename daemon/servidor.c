/*!
 * \file servidor.c
 * \brief Servidor single-thread interoperando com clientes ativos.
 * \date 08/12/2016
 * \author Gabriela Solano <gabriela.solano@aker.com.br>
 * 
 */

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
#include <limits.h>
#include <time.h>

#define BUFFERSIZE BUFSIZ
#define MAXCLIENTS 100

/*!
 * Struct para controle dos clientes ativos.
 */
typedef struct Monitoramento
{
	int sock;
	int enviar;
	unsigned long bytes_enviados;
	char caminho[BUFFERSIZE+1];
} monitoramento;

int existe_pagina (char *caminho);
int envia_cliente (int sock_cliente, char mensagem[], int size_strlen);
int envia_cabecalho (int indice_cliente, char cabecalho[], int flag_erro);
void formato_mesagem();
void inicia_servidor(int *sock, struct sockaddr_in *servidor, int porta);
void responde_cliente (int indice_cliente);
void recupera_caminho (int indice_cliente, char *pagina);
void envia_primeiro_buffer (int indice_cliente);
void envia_buffer (int indice_cliente);
void zera_struct_cliente (int indice_cliente);
void encerra_cliente (int indice_cliente);

struct Monitoramento clientes_ativos[MAXCLIENTS];
char diretorio[PATH_MAX+1];
int excluir_ativos = 0;
fd_set read_fds;
FILE *fp = NULL;

/*!
 * \brief Funcao principal que conecta o servidor e processa
 * requisicoes de clientes.
 * \param[in] argc Quantidade e parametros de entrada.
 * \param[in] argv[1] Porta de conexao do servidor.
 * \param[in] argv[2] Diretorio utilizado como root pelo servidor.
 */
int main (int argc, char **argv)
{
  struct sockaddr_in servidor;
  struct sockaddr_in cliente;
	struct timeval t_espera;
  socklen_t addrlen;
  int sock_servidor;
  int sock_cliente;
  int porta;
  int max_fd;
  int i;
	int ativos = 0;
	int pedido_cliente;

  /*! Valida quantidade de parametros de entrada */
  if (argc != 3)
  {
    printf("Quantidade incorreta de parametros.\n");
    formato_mesagem();
    return 1;
  }

  /*! Recupera e valida a porta */
  porta = atoi(argv[1]);
  if (porta < 0 || porta > 65535)
  {
    printf("Porta invalida!");
    formato_mesagem();
    return 1;
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
	t_espera.tv_sec = 1;
	t_espera.tv_usec = 0;

  /*! Loop principal para aceitar e lidar com conexoes do cliente */
  while (1)
  {
		FD_ZERO(&read_fds);
		FD_SET(sock_servidor, &read_fds);
		max_fd = sock_servidor;

    /*! Recebe uma nova conexao */
		if (ativos > 0) /*! Se houve alguma ativa, limita o tempo de espera */
    {
      pedido_cliente = select(max_fd + 1, &read_fds, NULL, NULL, &t_espera);
    }
    else
    {
			printf("\nEspera de cliente.\n");
      pedido_cliente = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
    }
		/*! Validacao do select */
    if (pedido_cliente == -1)
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
			}
			else
			{
				/*! Configuracao inicial de um cliente ativo */
				clientes_ativos[ativos].sock = sock_cliente;
				clientes_ativos[ativos].enviar = 1;
				clientes_ativos[ativos].bytes_enviados = 0;

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
		for (i = 0; i < MAXCLIENTS; i++)
		{
			/*! Verifica se esta esperando para leitura */
			if (FD_ISSET(clientes_ativos[i].sock, &read_fds))
			{
				/*! Se o cliente ja terminou sua leitura */
				if ((clientes_ativos[i].sock != -1)
								&& (clientes_ativos[i].enviar == 0))
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
				}
			}
		}
		ativos -= excluir_ativos;
  }
  close(sock_servidor);
  return 0;
}

/*!
 * \brief Encerra conexao com determinado cliente.
 * \param[in] indice_cliente Posicao do array de clientes ativos que determina
 * quem sera processado.
 */
void encerra_cliente (int indice_cliente)
{
	FD_CLR(clientes_ativos[indice_cliente].sock, &read_fds);
	printf("Encerrada conexao com socket %d\n",
					clientes_ativos[indice_cliente].sock);
	close(clientes_ativos[indice_cliente].sock);
	zera_struct_cliente(indice_cliente);
	excluir_ativos++;
}

/*!
 * \brief Atribui valores default para cliente presente no array de clientes
 * ativos. Posicoes zeradas no array sao considerados clientes inativos.
 * \param[in] indice_cliente Posicao do array de clientes ativos que sera 
 * modificada.
 */
void zera_struct_cliente (int indice_cliente)
{
	clientes_ativos[indice_cliente].sock = -1;
	clientes_ativos[indice_cliente].enviar = 0;
	clientes_ativos[indice_cliente].bytes_enviados = 0;
	memset(clientes_ativos[indice_cliente].caminho, '\0', BUFFERSIZE+1);
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

	/*! Conecta */
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
 * \brief Responde requisicao de determinado cliente.
 * \param[in] indice_cliente Indice do array de clientes ativos que determina
 * qual cliente deve ser respondido.
 */
void responde_cliente (int indice_cliente)
{
	if (clientes_ativos[indice_cliente].bytes_enviados == 0)
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
 * \brief Valida a requisicao do cliente para enviar o cabecalho HTTP
 * apropriado. Em caso de requisicao correta, tambem chama funcao para enviar
 * ao cliente um primeiro buffer (de tamanho maximo 8193 bytes) do arquivo
 * pedido.
 * \param[in] indice_cliente Indice no array de clientes ativos que determina
 * qual cliente esta sendo processado.
 */
void envia_primeiro_buffer (int indice_cliente)
{
	int bytes;
  int sock_cliente;
	int size_strlen;
	char buffer[BUFFERSIZE+1];
  char *http_metodo;
  char *http_versao;
  char *pagina;
  char *contexto;

	sock_cliente = clientes_ativos[indice_cliente].sock;
	memset(buffer, '\0', sizeof(buffer));
  if ((bytes = recv(sock_cliente, buffer, sizeof(buffer), 0)) > 0)
  {
    printf("Recebeu mensagem: %s\n", buffer);

    /*! Verifica se tem o GET no inicio */
    http_metodo = strtok_r(buffer, " ", &contexto);

		/*! Se houve o GET no inicio :  */
    if (strncmp(http_metodo, "GET", 3) == 0)
    {
      /*! Recupera a versao do protocolo HTTP : HTTP/1.0 ou HTTP/1.1 */
      pagina = strtok_r(NULL, " ", &contexto);
      http_versao = strtok_r(NULL, "\r", &contexto);

			/*! Se nao houve a versao do protocolo HTTP : request invalido */
			if (pagina == NULL || http_versao == NULL)
			{
				envia_cabecalho(indice_cliente, "HTTP/1.0 400 Bad Request\r\n\r\n", 1);
				clientes_ativos[indice_cliente].enviar = 0;
			}
      else if ((strncmp(http_versao, "HTTP/1.0", 8) != 0)
            && (strncmp(http_versao, "HTTP/1.1", 8) != 0))
      {
				envia_cabecalho(indice_cliente, "HTTP/1.0 400 Bad Request\r\n\r\n", 1);
				clientes_ativos[indice_cliente].enviar = 0;
      }
      /*! Casos de request valido: GET /path/to/file HTTP/1.0 (ou HTTP/1.1) */
      else
      {
        /*! Nao permite acesso fora do diretorio especificado */
				recupera_caminho(indice_cliente, pagina);
				size_strlen = strlen(diretorio);
        if (strncmp(diretorio, clientes_ativos[indice_cliente].caminho,
						size_strlen * sizeof(char)) != 0)
				{
					envia_cabecalho(indice_cliente, "HTTP/1.0 403 Forbidden\r\n\r\n", 1);
				}
        else
        {
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
								envia_buffer(indice_cliente);
							}
						}
          }
          else
          {
						envia_cabecalho(indice_cliente,
														"HTTP/1.0 404 Not Found\r\n\r\n", 1);
          }
        }
      }
    }
    /*! Se nao houver o GET no inicio : 501 Not Implemented */
    else
    {
			envia_cabecalho(indice_cliente,
											"HTTP/1.0 501 Not Implemented\r\n\r\n", 1);
    }
  }
  else if (bytes < 0)
  {
    perror("recv()");
		encerra_cliente (indice_cliente);
  }
  else if (bytes == 0)
  {
    printf("Socket %d encerrou a conexao.\n", sock_cliente);
		encerra_cliente (indice_cliente);
  }
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
	strncat(caminho, "/", sizeof(char));
	strncat(caminho, pagina, tam_pagina);

	realpath(caminho, clientes_ativos[indice].caminho);
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

/*!
 * Mensagem com o formato da entrada necessaria para a execucao do programa.
 */
void formato_mesagem ()
{
  printf("Formato: ./recuperador <porta> <diretorio>\n");
  printf("Portas validas: 0 a 65535\n");
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

/*!
 * \brief Envia um buffer do arquivo requisitado para o cliente. Atualiza o
 * estado do cliente no array de clientes ativos.
 * \param[in] indice_cliente Indice do array de clientes ativos que identifica
 * qual cliente esta sendo processado.
 */
void envia_buffer (int indice_cliente)
{
  char buf[BUFFERSIZE+1];
  unsigned int bytes_lidos = 0;
	unsigned long pular_buf = 0;
	int temp = 0;
	/*! Zera o buffer que vai receber o arquivo */
	memset(buf, '\0', sizeof(buf));

	/*! Inicia variaveis locais para melhorar leitura do codigo */
	pular_buf = clientes_ativos[indice_cliente].bytes_enviados;

	/*! Abre o arquivo */
	fp = fopen(clientes_ativos[indice_cliente].caminho, "rb");

	/*! Reposiciona o ponteiro para ler o arquivo */
	if (fseek(fp, pular_buf, SEEK_SET) != 0)
	{
		perror("fseek()");
		exit(1);
	}

	/*! Leitura de um buffer do arquivo */
	bytes_lidos = fread(buf, 1, sizeof(buf), fp);

	/*! O que leu foi menor que o tamanho do buffer, logo o arquivo acabou */
	if ((bytes_lidos > 0) && (bytes_lidos < sizeof(buf)))
	{
		temp = envia_cliente(indice_cliente, buf, bytes_lidos);
		if (temp == -1)
		{
			fclose(fp);
			return;
		}
		clientes_ativos[indice_cliente].enviar = 0;
	}
	else if (bytes_lidos == sizeof(buf))
	{
		/*! Leu um buffer inteiro, pode ser que exista mais coisa no arquivo */
		temp = envia_cliente(indice_cliente, buf, bytes_lidos);
		if (temp == -1)
		{
			fclose(fp);
			return;
		}
		clientes_ativos[indice_cliente].bytes_enviados += temp;
	}
	else
	{
		/*! Nao leu nada, arquivo acabou */
		clientes_ativos[indice_cliente].enviar = 0;
	}
	memset(buf, '\0', sizeof(buf));
	fclose(fp);
}
