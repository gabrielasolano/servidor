#include "estruturas.h"

/*!
 * \brief Inicializa filas e listas auxiliares
 */
void inicializa_estruturas()
{
	SLIST_INIT(&lista_arquivos);
	STAILQ_INIT(&fila_response_get_wait);
	STAILQ_INIT(&fila_request_get);
	STAILQ_INIT(&fila_response_get);
	STAILQ_INIT(&fila_request_put);
}

/*!
 * \brief Encerra filas e listas auxiliares
 */
void encerra_estruturas ()
{
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
	
	struct GetResponse *g3;
	while (!STAILQ_EMPTY(&fila_response_get_wait))
	{
		g3 = STAILQ_FIRST(&fila_response_get_wait);
		STAILQ_REMOVE_HEAD(&fila_response_get_wait, entry);
		free(g3);
	}

	struct PutRequest *p1;
	while (!STAILQ_EMPTY(&fila_request_put))
	{
		p1 = STAILQ_FIRST(&fila_request_put);
		STAILQ_REMOVE_HEAD(&fila_request_put, entry);
		free(p1);
	}
	
	struct Arquivos *paux;
	while (!SLIST_EMPTY(&lista_arquivos))
	{
		paux = SLIST_FIRST(&lista_arquivos);
		SLIST_REMOVE_HEAD(&lista_arquivos, entries);
		free(paux);
	}
}

/*!
 * \brief Insere novo elemento na fila de resposta para GET
 * \param[in] buf Mensagem da resposta
 * \param[in] indice Indice do cliente a qual sera respondido
 * \param[in] tam_buffer Tamanho da mensagem a ser respondida
 */
void insere_fila_response_get (char *buf, int indice, int tam_buffer)
{
	struct GetResponse *paux;

	paux = (struct GetResponse *) malloc(sizeof(struct GetResponse));
	paux->indice = indice;
	paux->tam_buffer = tam_buffer;
	memcpy(paux->buffer, buf, tam_buffer);

	STAILQ_INSERT_TAIL(&fila_response_get, paux, entry);
}

/*!
 * \brief Insere novo elemento na lista auxiliar para controle de banda
 * \param[in] buf Mensagem da resposta
 * \param[in] indice Indice do cliente a qual sera respondido
 * \param[in] tam_buffer Tamanho da mensagem a ser respondida
 */
void insere_fila_response_get_wait (char *buf, int indice, int tam_buffer)
{
	struct GetResponse *paux;

	paux = (struct GetResponse *) malloc(sizeof(struct GetResponse));
	paux->indice = indice;
	paux->tam_buffer = tam_buffer;
	memcpy(paux->buffer, buf, tam_buffer);

	STAILQ_INSERT_TAIL(&fila_response_get_wait, paux, entry);
}

/*!
 * \brief Insere novo elemento da fila de requests GET enviados pelos clientes
 * \param[in] indice Indice do cliente que enviou o request
 */
void insere_fila_request_get (int indice)
{
	struct GetRequest *paux;

	paux = (struct GetRequest *) malloc(sizeof(struct GetRequest));
	paux->indice = indice;
	STAILQ_INSERT_TAIL(&fila_request_get, paux, entry);

	/*! So acorda as threads uma vez */
	if (tamanho_fila_request_get() == 1)
	{
		pthread_mutex_lock(&mutex_master);
		pthread_cond_broadcast(&condition_master);
		pthread_mutex_unlock(&mutex_master);
	}
}

/*!
 * \brief Insere novo elemento da fila de requests PUT enviados pelos clientes
 * \param[in] indice Indice do cliente que enviou o request
 * \param[in] buf Mensagem enviado pelo cliente
 * \param[in] tam_buffer Tamanho da mensagem enviada pelo cliente
 * \param frame Numero do frame que o cliente enviou
 */
void insere_fila_request_put(int indice, char *buf, int tam_buffer,
														 unsigned long frame)
{
	struct PutRequest *paux;

	paux = (struct PutRequest *) malloc(sizeof(struct PutRequest));
	paux->indice = indice;
	paux->tam_buffer = tam_buffer;
	memcpy(paux->buffer, buf, tam_buffer);
	paux->frame = frame;

	STAILQ_INSERT_TAIL(&fila_request_put, paux, entry);

	/*! So acorda as threads uma vez */
	if (tamanho_fila_request_put() == 1)
	{
		pthread_mutex_lock(&mutex_master);
		pthread_cond_broadcast(&condition_master);
		pthread_mutex_unlock(&mutex_master);
	}
}

/*!
 * \brief Retira primeiro elemento da fila de requests PUT
 * \return Elemento retirado da fila (do tipo struct PutRequest)
 */
put_request *retira_fila_request_put ()
{
	struct PutRequest *retorno;

	retorno = STAILQ_FIRST(&fila_request_put);
	STAILQ_REMOVE_HEAD(&fila_request_put, entry);

	return retorno;
}

/*!
 * \brief Retira primeiro elemento da fila de requests GET
 * \return Elemento retirado da fila (do tipo struct GetRequest)
 */
get_request *retira_fila_request_get()
{
	struct GetRequest *retorno;

	retorno = STAILQ_FIRST(&fila_request_get);
	STAILQ_REMOVE_HEAD(&fila_request_get, entry);

	return retorno;
}

/*!
 * \brief Retira primeiro elemento da fila auxiliar para controle de banda de
 * respostas GET
 * \return Elemento retirado da fila (do tipo struct GetResponse)
 */
get_response *retira_fila_response_get_wait()
{
	struct GetResponse *retorno;

	retorno = STAILQ_FIRST(&fila_response_get_wait);
	STAILQ_REMOVE_HEAD(&fila_response_get_wait, entry);

	return retorno;
}

/*!
 * \brief Retira primeiro elemento da fila de respostas GET
 * \return Elemento retirado da fila (do tipo struct GetResponse)
 */
get_response *retira_fila_response_get()
{
	struct GetResponse *retorno;

	retorno = STAILQ_FIRST(&fila_response_get);
	STAILQ_REMOVE_HEAD(&fila_response_get, entry);

	return retorno;
}

/*!
 * \brief Calcula o tamanho da fila de GET requests
 * \return Tamanho da fila
 */
int tamanho_fila_request_get ()
{
	struct GetRequest *paux;
	int tamanho = 0;
	
	STAILQ_FOREACH(paux, &fila_request_get, entry)
		tamanho++;

	return tamanho;
}

/*!
 * \brief Calcula o tamanho da fila de PUT requests
 * \return Tamanho da fila
 */
int tamanho_fila_request_put ()
{
	struct PutRequest *paux;
	int tamanho = 0;
	
	STAILQ_FOREACH(paux, &fila_request_put, entry)
		tamanho++;

	return tamanho;
}

/*!
 * \brief Verifica se a fila de GET requests esta vazia
 * \return 0 se nao vazia
 * \return 1 se vazia
 */
int fila_request_get_vazia ()
{
	return STAILQ_EMPTY(&fila_request_get);
}

/*!
 * \brief Verifica se a fila de PUT requests esta vazia
 * \return 0 se nao vazia
 * \return 1 se vazia
 */
int fila_request_put_vazia ()
{
	return STAILQ_EMPTY(&fila_request_put);
}

/*!
 * \brief Verifica se o arquivo pedido pelo cliente ja esta sendo utilizado em
 * algum request PUT
 * \param[in] indice Indice do cliente no array de clientes ativos
 * \param[in] arquivo_is_put Flag que indica se o request do cliente 'e PUT
 * \return 0 Arquivo pode ser utilizado e deve ser inserido na lista
 * \return 1 Arquivo pode ser utilizado mas ja esta inserido na lista
 * \return -1 Arquivo nao pode ser utilizado
 */
int arquivo_pode_utilizar (int indice, int arquivo_is_put)
{
	struct Arquivos *paux;
	int size_strlen;
	
	size_strlen = strlen(clientes[indice].caminho);
	if (SLIST_EMPTY(&lista_arquivos))
	{
		return 0;
	}
	else
	{
		SLIST_FOREACH(paux, &lista_arquivos, entries)
		{
			if (strncmp(paux->caminho, clientes[indice].caminho, size_strlen) == 0)
			{
				if (paux->arquivo_is_put || arquivo_is_put)
				{
					return -1;
				}
				return 1;
			}
		}
	}
	return 0;
}

/*!
 * \brief Insere um novo arquivo na lista de arquivos abertos
 * \param[in] indice Indice do cliente que requisitou o arquivo
 * \param[in] arquivo_is_put Flag que indica se o arquivo novo 'e request PUT
 */
void insere_lista_arquivos(int indice, int arquivo_is_put)
{
	struct Arquivos *pnew;
	int size_strlen;

	size_strlen = strlen(clientes[indice].caminho);
	
	pnew = malloc(sizeof(struct Arquivos));
	
	strncpy(pnew->caminho, clientes[indice].caminho, size_strlen);
	pnew->socket = clientes[indice].sock;
	pnew->arquivo_is_put = arquivo_is_put;
	pnew->indice = indice;
	
	SLIST_INSERT_HEAD(&lista_arquivos, pnew, entries);
}

/*!
 * \brief Remove um arquivo da lista de arquivos abertos
 * \param[in] indice Indice do cliente que requisitou o arquivo
 */
void remove_arquivo_lista (int indice)
{
	struct Arquivos *paux;
	
	SLIST_FOREACH(paux, &lista_arquivos, entries)
	{
		if (paux->indice == indice)
		{
			SLIST_REMOVE(&lista_arquivos, paux, Arquivos, entries);
			free(paux);
			return;
		}
	}
}
