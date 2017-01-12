#include "threads.h"

pthread_t threads[NUM_THREADS];

/*!
 * \brief Cria threads trabalhadoras
 */
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

/*!
 * \brief Funcao principal executada pelas threads trabalhadoras. Cria um
 * socket local para comunicacao com o servidor. Espera sinal da threads
 * principal para fazer leitura/escrita dos arquivos do cliente. Avisa a thread
 * principal quando termina seu trabalho.
 */
void *funcao_thread (void *id)
{
	int indice;
	int tam_buffer;
	int sock_local;
	int bytes_lidos;
	unsigned long frame;
	char buffer[BUFFERSIZE+1];
	char enviar[3] = "ok";
	struct sockaddr_un addr_local;

	sock_local = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock_local == -1)
	{
		perror("sock local()");
		return NULL;
	}

	addr_local.sun_family = AF_UNIX;
	memcpy(addr_local.sun_path, sock_path, sizeof(addr_local.sun_path));

	while (1)
	{
		pthread_mutex_lock(&mutex_master);
		//fprintf(fp_log, "mutex_master LOCKED em funcao_threads(1) threads.c pela thread %ld\n", (long) id);

		while (fila_request_get_vazia() && fila_request_put_vazia() && (quit == 0))
		{
			pthread_cond_wait(&condition_master, &mutex_master);
		}
		pthread_mutex_unlock(&mutex_master);
// 		fprintf(fp_log, "mutex_master UNLOCKED em funcao_threads(1) threads.c pela thread %ld\n", (long) id);

		if (quit)
		{
			close(sock_local);
			return NULL;
		}

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

			pthread_mutex_lock(&clientes_threads[indice].mutex);
			
			if (clientes[indice].tam_arquivo == clientes[indice].bytes_lidos)
			{
				pthread_mutex_unlock(&clientes_threads[indice].mutex);
				close(sock_local);
				return NULL;
			}

			fseek(clientes[indice].fp, clientes[indice].bytes_lidos, SEEK_SET);

			bytes_lidos = fread(buffer, 1, buffer_size, clientes[indice].fp);
			
			if (bytes_lidos <= 0)
			{
				pthread_mutex_unlock(&clientes_threads[indice].mutex);
				close(sock_local);
				return NULL;
			}
			else
			{
				clientes[indice].bytes_lidos += bytes_lidos;
				clientes[indice].frame_recebido++;

				pthread_mutex_lock(&mutex_fila_response_get);
				insere_fila_response_get(buffer, indice, bytes_lidos);
				pthread_mutex_unlock(&mutex_fila_response_get);
				
				pthread_mutex_unlock(&clientes_threads[indice].mutex);

				if (sendto(sock_local, enviar, sizeof(enviar), 0, (struct sockaddr *)
										&addr_local, sizeof(struct sockaddr_un)) == -1)
				{
					perror("sendto()");
					close(sock_local);
					return NULL;
				}
			}
		}

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

			pthread_mutex_lock(&clientes_threads[indice].mutex);

			while (frame != clientes[indice].frame_escrito)
			{
				pthread_cond_wait(&clientes_threads[indice].cond,
													&clientes_threads[indice].mutex);
			}

			bytes_lidos = fwrite(buffer, tam_buffer, 1, clientes[indice].fp);

			if (bytes_lidos < 0)
			{
				pthread_mutex_unlock(&clientes_threads[indice].mutex);
				close(sock_local);
				return NULL;
			}
			else
			{
				clientes[indice].bytes_lidos += (tam_buffer * bytes_lidos);
				clientes[indice].bytes_por_envio += (tam_buffer * bytes_lidos);
				clientes[indice].frame_escrito++;
				if (clientes[indice].bytes_lidos >= clientes[indice].tam_arquivo)
				{
					clientes[indice].quit = 1;
				}
			}
			pthread_cond_broadcast(&clientes_threads[indice].cond);
			pthread_mutex_unlock(&clientes_threads[indice].mutex);
		}
	}
	close(sock_local);
	return NULL;
}
