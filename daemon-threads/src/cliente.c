#include "cliente.h"

struct Monitoramento cliente_vazio = {0};

/*!
 * \brief Atribui valores default para o cliente indicado
 * \param[in] indice Indice do cliente no array de clientes ativos
 */
void zera_struct_cliente (int indice)
{
	clientes[indice] = cliente_vazio;
	clientes[indice].sock = -1;
	timerclear(&clientes[indice].t_cliente);
}

/*!
 * \brief Encerra o cliente indicado. Atualiza a quantidade de clientes ativos
 * \param[in] indice Indice do cliente no array de clientes ativos
 */
void encerra_cliente (int indice)
{
	if (clientes[indice].sock != -1)
	{
		remove_arquivo_lista(indice);
		FD_CLR(clientes[indice].sock, &read_fds);
		printf("Encerrada conexao com socket %d\n",
						clientes[indice].sock);
		if (clientes[indice].fp != NULL)
		{
			fclose(clientes[indice].fp);
		}
		close(clientes[indice].sock);
		zera_struct_cliente(indice);
		ativos--;
		if (ativos < 0)
			ativos = 0;
	}
}

/*!
 * \brief Envia resposta ao cliente indicado. Controla velocidade se precisar
 * \param[in] indice Indice do cliente no array de clientes ativos
 * \param[in] mensagem Mensagem enviado ao clientes
 * \param[in] size Tamanho da mensagem enviada ao cliente
 */
void envia_cliente (int indice, char mensagem[], int size)
{
	int enviado;
	int enviar;

	clientes[indice].pode_enviar = buffer_size;

	if (controle_velocidade)
	{
		enviar = controle_banda(indice);

		if (!enviar)
		{
			insere_fila_response_get_wait(mensagem, indice, size);
			return;
		}
		gettimeofday(&clientes[indice].t_cliente, NULL);
	}

	if ((unsigned) size > clientes[indice].pode_enviar)
	{
		size = clientes[indice].pode_enviar;
	}
	enviado = send(clientes[indice].sock, mensagem, size, MSG_NOSIGNAL);

	if (enviado < 0)
	{
    perror("send()");
		encerra_cliente(indice);
		return;
	}
	clientes[indice].bytes_enviados += enviado;
	clientes[indice].bytes_lidos = clientes[indice].bytes_enviados;
	clientes[indice].bytes_por_envio += enviado;
	clientes[indice].frame_escrito++;

	if (clientes[indice].bytes_enviados
				>= (unsigned) clientes[indice].tam_arquivo)
	{
		encerra_cliente(indice);
	}
	else
	{
		pthread_mutex_lock(&mutex_fila_request_get);
		insere_fila_request_get(indice);
		pthread_mutex_unlock(&mutex_fila_request_get);
	}
}

/*!
 * \brief Parser do cabecalho recebido do cliente
 * \param[in] indice Indice do cliente no array de cliente ativos
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

	memset(buffer, '\0', sizeof(buffer));
	size_strlen = strlen(clientes[indice].cabecalho);
	strncpy(buffer, clientes[indice].cabecalho, size_strlen);
  printf("Recebeu mensagem: %s\n", buffer);

  http_metodo = strtok_r(buffer, " ", &contexto);
	pagina = strtok_r(NULL, " ", &contexto);
  http_versao = strtok_r(NULL, "\r", &contexto);

	if (pagina == NULL || http_versao == NULL)
	{
		envia_cabecalho(indice, "HTTP/1.0 400 Bad Request\r\n\r\n", 1);
		return;
	}
  else if ((strncmp(http_versao, "HTTP/1.0", 8) != 0)
						&& (strncmp(http_versao, "HTTP/1.1", 8) != 0))
  {
		envia_cabecalho(indice, "HTTP/1.0 400 Bad Request\r\n\r\n", 1);
		return;
	}

	if (recupera_caminho(indice, pagina) == -1)
	{
		envia_cabecalho(indice, "HTTP/1.0 400 Bad Request\r\n\r\n", 1);
		return;
	}
	size_strlen = strlen(diretorio);
  if (strncmp(diretorio, clientes[indice].caminho, size_strlen * sizeof(char))
				!= 0)
	{
		envia_cabecalho(indice, "HTTP/1.0 403 Forbidden\r\n\r\n", 1);
		return;
	}

  if (strncmp(http_metodo, "GET", 3) == 0)
  {
		inserir = arquivo_pode_utilizar(indice, 0);
		if (inserir == -1)
		{
			envia_cabecalho(indice, "HTTP/1.0 503 Service Unavailable\r\n\r\n", 1);
			return;
		}
		else if (inserir == 0)
		{
			insere_lista_arquivos(indice, 0);
		}
		cabecalho_get(indice);
	}
	else if (strncmp(http_metodo, "PUT", 3) == 0)
	{
		inserir = arquivo_pode_utilizar(indice, 1);
		if (inserir == -1)
		{
			envia_cabecalho(indice, "HTTP/1.0 503 Service Unavailable\r\n\r\n", 1);
			return;
		}
		else if (inserir == 0)
		{
			insere_lista_arquivos(indice, 1);
		}
		cabecalho_put(indice);
	}
  else
  {
		envia_cabecalho(indice, "HTTP/1.0 501 Not Implemented\r\n\r\n", 1);
		return;
  }
}

/*!
 * \brief Recebe o cabecalho do cliente
 * \param[in] indice Indice do cliente no array de clientes ativos
 */
void recebe_cabecalho_cliente (int indice)
{
	char buffer[BUFFERSIZE+1];
	char mensagem[BUFFERSIZE+1];
	int bytes;
	int sock;
	int estado = 0;
	int fim_cabecalho = 0;

	memset(buffer, 0, sizeof(buffer));
	memset(mensagem, '\0', sizeof(mensagem));

	sock = clientes[indice].sock;
	while ((!fim_cabecalho) && ((bytes = recv(sock, buffer, 1, 0)) > 0))
	{
		memcpy(clientes[indice].cabecalho + clientes[indice].tam_cabecalho, buffer,
						bytes);
		clientes[indice].tam_cabecalho += bytes;
		if (((estado == 0 || estado == 2) && (buffer[0] == '\r'))
					|| (estado == 1 && buffer[0] == '\n'))
		{
			estado++;
		}
		else if (estado == 3 && buffer[0] == '\n')
		{
			fim_cabecalho = 1;
			clientes[indice].recebeu_cabecalho = 1;
		}
		else
		{
			estado = 0;
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
		printf("Socket %d encerrou a conexao.\n", sock);
		encerra_cliente (indice);
	}
	else
	{
		cabecalho_parser(indice);
	}
}

/*!
 * \brief Recebe arquivo enviado pelo cliente (PUT request). Faz controle de 
 * velocidade se precisar.
 * \param[in] indice Indice do cliente no array de clientes ativos
 */
void recebe_arquivo_put (int indice)
{
	int bytes;
	char buf[BUFFERSIZE+1];
	int sock;
	int enviar;

	clientes[indice].pode_enviar = buffer_size;

	if (controle_velocidade)
	{
		enviar = controle_banda(indice);
	
		if (!enviar)
		{
			return;
		}
		gettimeofday(&clientes[indice].t_cliente, NULL);
	}

	if ((unsigned) buffer_size > clientes[indice].pode_enviar)
	{
		buffer_size = clientes[indice].pode_enviar;
	}

	sock = clientes[indice].sock;

	bytes = recv(sock, buf, buffer_size, MSG_DONTWAIT);
	
	if (clientes[indice].quit == 1)
	{
		return;
	}

	if (bytes > 0)
	{
		clientes[indice].bytes_enviados += bytes;
		clientes[indice].bytes_por_envio += bytes;

		pthread_mutex_lock(&mutex_fila_request_put);
		insere_fila_request_put(indice, buf, bytes,
														clientes[indice].frame_recebido);
		pthread_mutex_unlock(&mutex_fila_request_put);
		clientes[indice].frame_recebido++;
	}
	else if (bytes < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			perror("recv()");
			clientes[indice].quit = 1;
		}
	}
	else if (bytes == 0)
	{
		printf("Socket %d encerrou a conexao.\n", sock);
		clientes[indice].quit = 1;
	}
}

/*!
 * \brief Recupera path do arquivo requisitado pelo clientes_ativos
 * \param[in] indice Indice do cliente no array de clientes ativos
 * \param[in] pagina Arquivo requisitado pelo clientes_ativos
 * \return -1 Se path nao se referir ao de um arquivo
 * \return 1 Se path correto
 */
int recupera_caminho (int indice, char *pagina)
{
	int tam_pagina = strlen(pagina);
	int tam_diretorio = strlen(diretorio);
	char caminho[PATH_MAX+1];

	memset(caminho, '\0', sizeof(caminho));
	strncpy(caminho, diretorio, tam_diretorio);
	strncat(caminho, pagina, tam_pagina);

	realpath(caminho, clientes[indice].caminho);

	/*! Nao permite diretorio */
	int size = strlen(clientes[indice].caminho) - 1;
	if (strncmp(clientes[indice].caminho+size, "/", 1) == 0)
	{
		return -1;
	}

	return 1;
}

/*!
 * \brief Envia cabecalho de resposta ao clientes
 * \param[in] indice Indice do cliente no array de clientes ativos
 * \param[in] cabecalho Cabecalho enviado ao cliente
 * \param[in] flag_erro Flag que indica se o cabecalho enviado foi erro HTTP
 * \return -1 Erro no envio
 * \return 0 Envio correto
 */
int envia_cabecalho (int indice, char cabecalho[], int flag_erro)
{
	int size_strlen;
	int temp;
	size_strlen = strlen(cabecalho);
	temp = send(clientes[indice].sock, cabecalho, size_strlen, MSG_NOSIGNAL);

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

/*!
 * \brief Verifica se o path indicado 'e um diretorio
 * \param[in] caminho Path a ser verificado
 * \return 0 Nao 'e diretorio 
 * \return 1 'E diretorio
 */
int caminho_diretorio (char *caminho)
{
	struct stat buffer = {0};
	stat(caminho, &buffer);
	return S_ISDIR(buffer.st_mode);
}

/*!
 * \brief Validacao do cabecalho de requisicao GET
 * \param[in] indice Indice do cliente no array de clientes ativos
 */
void cabecalho_get (int indice)
{
	if ((!caminho_diretorio(clientes[indice].caminho))
				&& existe_pagina(clientes[indice].caminho))
  {
		clientes[indice].fp = fopen(clientes[indice].caminho, "rb");

		if (clientes[indice].fp == NULL)
		{
			perror("fopen()");
			envia_cabecalho(indice, "HTTP/1.0 404 Not Found\r\n\r\n", 1);
		}
		else
		{
			struct stat st;
			stat(clientes[indice].caminho, &st);
			clientes[indice].tam_arquivo = st.st_size;

			if (envia_cabecalho(indice, "HTTP/1.0 200 OK\r\n\r\n", 0) == 0)
			{
				pthread_mutex_lock(&mutex_fila_request_get);
				insere_fila_request_get(indice);
				pthread_mutex_unlock(&mutex_fila_request_get);
			}
		}
	}
  else
  {
		envia_cabecalho(indice, "HTTP/1.0 404 Not Found\r\n\r\n", 1);
	}
}

/*!
 * \brief Recupera o tamanho do arquivo pelo cabecalho PUT
 * \param[in] indice Indice do cliente no array de clientes ativos
 * \return 0 Nao foi possivel recuperar o tamanho
 * \return 1 Tamanho recuperado corretamente
 */
int recupera_tam_arquivo (int indice)
{
	char cabecalho[BUFFERSIZE+1];
	char content_length[20] = "Content-Length: ";
	char numero_tamanho[100];
	char *recuperar;
	int tam_cabecalho;
	int tam_content;
	int i;

	tam_cabecalho = strlen(clientes[indice].cabecalho);
	tam_content = strlen(content_length);
	strncpy(cabecalho, clientes[indice].cabecalho, tam_cabecalho);

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
	clientes[indice].tam_arquivo = tam_arquivo;

	return 1;
}

/*!
 * \brief Validacao do cabecalho de requisicao PUT
 * \param[in] indice Indice do cliente no array de clientes ativos
 */
void cabecalho_put (int indice)
{
	FILE *put_result = NULL;
	int temp;

	if(!recupera_tam_arquivo(indice))
	{
		envia_cabecalho(indice, "HTTP/1.1 411 Length Required\r\n\r\n", 1);
		return;
	}

	if ((!caminho_diretorio(clientes[indice].caminho))
				&& (existe_diretorio(clientes[indice].caminho)))
	{
		if (existe_pagina(clientes[indice].caminho))
		{
			put_result = fopen(clientes[indice].caminho, "rb");
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
		else
		{
			temp = envia_cabecalho(indice, "HTTP/1.0 201 Created\r\n\r\n", 0);
			if (temp == -1)
			{
				return;
			}
		}
		clientes[indice].fp = fopen(clientes[indice].caminho, "wb");
		if (clientes[indice].tam_arquivo == 0)
		{
			clientes[indice].quit = 1;
		}
	}
	else
  {
		envia_cabecalho(indice, "HTTP/1.0 404 Not Found\r\n\r\n", 1);
	}
}

/*!
 * \brief Verifica se o path indicado existe
 * \param[in] caminho Path a ser verificado
 * \return 0 Path nao existente
 * \return 1 Path existente
 */
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
 * \brief Controle a banda e tempo de envio/recebimento para cada cliente
 * \param[in] indice Indice do cliente no array de clientes ativos
 * \return 1 Thread principal pode enviar/receber mais conteudo
 * \return 0 Threda principal nao pode mais enviar/receber
 */
int controle_banda (int indice)
{
	struct timeval t_atual;
	struct timeval t_aux;
	struct timeval t_segundo;

	if (!timerisset(&clientes[indice].t_cliente))
	{
		return 1;
	}

	gettimeofday(&t_atual, NULL);
	t_segundo.tv_sec = 1;
	t_segundo.tv_usec = 0;
	timersub(&t_atual, &clientes[indice].t_cliente, &t_aux);	

	if (timercmp(&t_aux, &t_segundo, <))
	{
		if (clientes[indice].bytes_por_envio >= (unsigned long) banda_maxima)
		{
			return 0;
		}
		long temp = banda_maxima - clientes[indice].bytes_por_envio;
		if ((temp > 0) && (temp < BUFFERSIZE+1))
		{
			clientes[indice].pode_enviar = temp;
		}
	}
	else
	{
		clientes[indice].bytes_por_envio = 0;
		clientes[indice].pode_enviar = buffer_size;
	}
	return 1;
}

/*!
 * \brief Verifica se a pagina requisitada pelo cliente existe_pagina
 * \param[in] caminho Path da pagina a ser verificada
 * \return 0 Pagina nao existe
 * \return 1 Pagina existe
 */
int existe_pagina (char *caminho)
{
  struct stat buffer;
  return (stat (caminho, &buffer) == 0);
}
