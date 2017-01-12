#include "servidor.h"

int porta_atual;
int max_fd;
int sock_servidor;
int sock_thread;
struct sockaddr_in servidor_addr;
struct sockaddr_un thread_addr;
struct timeval timeout;

/*!
 * \brief Funcao principal do servidor. Cria, configura e conecta os sockets do
 * servidor. Inicializa estruturas auxiliares. Cria threads. Recebe, aceita e 
 * processa novos clientes.
 * \param[in] porta Porta para conexao do servidor
 */
void funcao_principal (int porta)
{
  int i;
	int pedido_cliente;
	int recebido_from = 0;

	signal(SIGINT, signal_handler);
	signal(SIGUSR1, signal_interface);

	/*! Atualiza a buffer_size */
	atualiza_buffer_size();

  printf("Iniciando servidor na porta: %d e no path: %s\n", porta, diretorio);
  inicia_servidor(porta);
	porta_atual = porta;
	
	/*! Inicializa struct dos clientes e dos mutexes */
	for (i = 0; i < MAXCLIENTS; i++)
	{
		zera_struct_cliente(i);
		pthread_mutex_init(&clientes_threads[i].mutex, NULL);
		pthread_cond_init(&clientes_threads[i].cond, NULL);
	}

	/*! Inicializa mutex e condition master */
	pthread_mutex_init(&mutex_master, NULL);
	pthread_cond_init (&condition_master, NULL);

	/*! Inicializa estruturas de dados auxiliares */
	inicializa_estruturas();

	/*! Inicializa mutexes das filas auxiliares */
	pthread_mutex_init(&mutex_fila_request_get, NULL);
	pthread_mutex_init(&mutex_fila_response_get, NULL);
	pthread_mutex_init(&mutex_fila_request_put, NULL);

	timerclear(&timeout);
	max_fd = sock_servidor;

	cria_threads();

  /*! Loop principal para aceitar e lidar com conexoes do cliente */
  while (1)
  {
		FD_ZERO(&read_fds);
		FD_SET(sock_servidor, &read_fds);

		if (ativos > 0)
    {
      pedido_cliente = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
    }
    else
    {
			printf("\nEspera de cliente.\n");
      pedido_cliente = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
    }

    if ((pedido_cliente < 0) && (errno != EINTR))
		{
			perror("select()");
			encerra_servidor();
		}

		if (quit)
		{
			encerra_servidor();
		}

		if (alterar_config)
		{
			atualiza_servidor();
			printf("Servidor na porta: %d, path: %s e com %ld B/s de velocidade\n",
						 porta_atual, diretorio, banda_maxima);
			alterar_config = 0;
		}
		
		atualiza_readfd();

		if (!aceita_conexoes(pedido_cliente))
		{
			break;
		}

		recebido_from = recebe_sinal_threads();

		processa_clientes(recebido_from);
  }
  close(sock_servidor);
	close(sock_thread);
}

/*!
 * \brief Atualiza o tamanho do buffer de acordo com o limite de banda.
 */
void atualiza_buffer_size ()
{
	controle_velocidade = 0;
	if (banda_maxima != 0)
	{
		controle_velocidade = 1;
		if (banda_maxima < BUFFERSIZE+1)
		{
			buffer_size = banda_maxima;
		}
		else
		{
			buffer_size = BUFFERSIZE+1;
		}
	}
	else
	{
		buffer_size = BUFFERSIZE+1;
	}
}

/*!
 * \brief Itera sobre cada cliente para enviar/receber mensagens ou para
 * encerra-los
 * \param[in] recebido_from Flag que indica se o recvfrom falhou, para poder
 * processar a fila auxiliar de controle de velocidade para requests GET
 */
void processa_clientes(int recebido_from)
{
	int i;

	for (i = 0; i < MAXCLIENTS; i++)
	{
		if ((recebido_from == -1) &&
					(clientes[i].frame_recebido > clientes[i].frame_escrito))
		{
			if (!STAILQ_EMPTY(&fila_response_get_wait))
			{
				get_response *r_aux = retira_fila_response_get_wait();
				envia_cliente(r_aux->indice, r_aux->buffer, r_aux->tam_buffer);
				free(r_aux);				
			}
		}
		else if ((clientes[i].sock != -1)
							&& (FD_ISSET(clientes[i].sock, &read_fds)))
		{
			if (clientes[i].recebeu_cabecalho == 0)
			{
				recebe_cabecalho_cliente(i);
			}
			else if (strncmp(clientes[i].cabecalho, "PUT", 3) == 0)
			{
				recebe_arquivo_put(i);
			}
		}
	}
}

/*!
 * \brief Funcao que recebe sinais das threads trabalhadoras por meio de
 * recvfrom
 * \return -1 Caso nao tenha recebido sinais
 * \return Quantidade de bytes recebidos
 */
int recebe_sinal_threads()
{
	socklen_t addrlen;
	int retorno;
	char recebido[3];
	
	addrlen = sizeof(thread_addr);
	while ((retorno = recvfrom(sock_thread, recebido, sizeof(recebido),
		MSG_DONTWAIT, (struct sockaddr *) &thread_addr, &addrlen)) > 0)
	{
		pthread_mutex_lock(&mutex_fila_response_get);
		get_response *r_aux = retira_fila_response_get();
		pthread_mutex_unlock(&mutex_fila_response_get);
		envia_cliente(r_aux->indice, r_aux->buffer, r_aux->tam_buffer);
		free(r_aux);
	}
	return retorno;
}

/*!
 * \brief Aceita novas conexoes de clientes
 * \param pedido_cliente Flag que indica se o servidor escutou algum cliente
 * novo para aceita-lo
 * \return 0 em erro do accept
 * \return 1 caso accept ocorreu sem erros
 */
int aceita_conexoes (int pedido_cliente)
{
	struct sockaddr_in cliente;
  socklen_t addrlen;
  int sock_cliente;

	if ((pedido_cliente > 0) && FD_ISSET(sock_servidor, &read_fds)
				&& (ativos < MAXCLIENTS))
	{
		addrlen = sizeof(cliente);
		sock_cliente = accept(sock_servidor, (struct sockaddr *) &cliente,
													&addrlen);
		if (sock_cliente == -1)
		{
			perror("accept()");
			return 0;
		}
		else
		{
			clientes[ativos].sock = sock_cliente;
			clientes[ativos].pode_enviar = buffer_size;

			ativos++;

			FD_SET(sock_cliente, &read_fds);
			if (sock_cliente > max_fd)
			{
				max_fd = sock_cliente;
			}
			printf("Nova conexao de %s no socket %d\n", inet_ntoa(cliente.sin_addr),
						 (sock_cliente));
		}
	}
	return 1;
}

/*!
 * \brief Atualiza a lista de clientes ativos para leitura
 */
void atualiza_readfd ()
{
	int i;

	for (i = 0; i < MAXCLIENTS; i++)
	{
		if (clientes[i].quit == 1)
		{
			encerra_cliente(i);
			break;
		}
		if (clientes[i].sock != -1)
		{
			FD_SET(clientes[i].sock, &read_fds);
			if (clientes[i].sock > max_fd)
				max_fd = clientes[i].sock;
		}
	}
}

/*!
 * \brief Inicia servidor nas conexoes UDP e TCP
 * \param[in] porta Porta para conexao TCP
 */
void inicia_servidor (const int porta)
{

	/*! Conexao TCP */
	atualiza_porta(porta);

	/*! Conexao UDP */
	sock_thread = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock_thread == -1)
	{
		perror("socket thread()");
		exit(1);
	}
	
	thread_addr.sun_family = AF_UNIX;
	memcpy(thread_addr.sun_path, sock_path, sizeof(thread_addr.sun_path));
	unlink(thread_addr.sun_path);

	if(bind(sock_thread, (struct sockaddr *) &thread_addr, sizeof(thread_addr))
			== -1)
	{
		perror("bind thread()");
		exit(1);
	}
}

void atualiza_porta (const int porta)
{
	sock_servidor = socket(PF_INET, SOCK_STREAM, 0);
  if (sock_servidor == -1)
  {
    perror("socket()");
		exit(1);
  }

  int yes = 1;
  if (setsockopt(sock_servidor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
			== -1)
  {
    perror("setsockopt()");
		exit(1);
  }

  servidor_addr.sin_family = AF_INET;
  servidor_addr.sin_port = htons(porta);
  servidor_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(&(servidor_addr.sin_zero), '\0', 8);

  if (bind(sock_servidor, (struct sockaddr *) &servidor_addr,
			sizeof(struct sockaddr)) == -1)
  {
    perror("bind()");
		exit(1);
  }

  if (listen(sock_servidor, MAXCLIENTS) == -1)
  {
    perror("listen()");
		exit(1);
  }
}

/*!
 * \brief Recupera as configuracoes enviadas pela gui
 */
void atualiza_servidor ()
{
	FILE *fp;
	int porta_gui = 0;
	int size;
	int size_gui;
	char diretorio_gui[PATH_MAX+1];
	long banda_gui = 0;

	/*! Recupera arquivo CONFIG_PATH */
	fp = fopen(config_path, "r");
	if (fp == NULL)
	{
		perror("Atualizacao servidor");
		return;
	}

	fscanf(fp, "%d", &porta_gui);
	fscanf(fp, "%s", diretorio_gui);
	fscanf(fp, "%ld", &banda_gui);

	fclose(fp);

	/*! Atualiza a porta se for diferente */
	if (porta_atual != porta_gui)
	{
		close(sock_servidor);
		atualiza_porta(porta_gui);
		porta_atual = porta_gui;
	}

	/*! Atualiza o diretorio */
	size = strlen(diretorio);
	size_gui = strlen(diretorio_gui);
	if(size != size_gui)
	{
		memset(diretorio, 0, size);
		memcpy(diretorio, diretorio_gui, size_gui);
	}
	else if (strncmp(diretorio, diretorio_gui, size) != 0)
	{
		memset(diretorio, 0, size);
		memcpy(diretorio, diretorio_gui, size);
	}
	
	if (chdir(diretorio) == -1)
	{
		perror("chdir()");
		quit = 1;
		encerra_servidor();
	}

	getcwd(diretorio, sizeof(diretorio));
	if (diretorio == NULL)
	{
		perror("getcwd()");
		quit = 1;
		encerra_servidor();
	}

	/*! Atualiza a banda maxima */
	if (banda_maxima != banda_gui)
	{
		banda_maxima = banda_gui;
		atualiza_buffer_size();
	}

	escreve_arquivo_config(porta_atual, diretorio, banda_maxima);
}

/*!
 * \brief Encerra o servidor e libera toda a memoria alocada na execucao do 
 * programa
 */
void encerra_servidor ()
{
	int i;

	for (i = 0; i < MAXCLIENTS; i++)
	{
		pthread_mutex_destroy(&clientes_threads[i].mutex);
		pthread_cond_destroy(&clientes_threads[i].cond);
	}

	pthread_mutex_lock(&mutex_master);
	pthread_cond_broadcast(&condition_master);
	pthread_mutex_unlock(&mutex_master);

	for (i = 0; i < NUM_THREADS; i++)
	{
		pthread_join(threads[i], NULL);
	}

	pthread_mutex_destroy(&mutex_master);
	pthread_cond_destroy(&condition_master);
	pthread_mutex_destroy(&mutex_fila_request_get);
	pthread_mutex_destroy(&mutex_fila_response_get);
	pthread_mutex_destroy(&mutex_fila_request_put);

	for (i = 0; i < MAXCLIENTS; i++)
	{
		encerra_cliente(i);
	}

	encerra_estruturas();

	remove(sock_path);
	remove(config_path);
	remove(pid_path);

	close(sock_servidor);
	close(sock_thread);
	pthread_exit(NULL);
	exit(1);
}

/*!
 * \brief Processa sinal SIGINT recebido pelo servidor
 */
void signal_handler (int signum)
{
	printf("Recebido sinal %d\n", signum);
	quit = 1;
}

/*!
 * \brief Processa sinal SIGINT recebido pelo servidor
 */
void signal_interface (int signum)
{
	printf("Recebido sinal da interface: %d\n", signum);
	alterar_config = 1;
}
