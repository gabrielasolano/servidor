#ifndef ESTRUTURAS_H
#define ESTRUTURAS_H

#include "funcoes_comuns.h"

typedef struct Arquivos
{
	char caminho[PATH_MAX+1];
	int arquivo_is_put;
	int socket;
	int indice;
	SLIST_ENTRY(Arquivos) entries;
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

void inicializa_estruturas();
void encerra_estruturas();
void insere_fila_response_get (char *buf, int indice, int tam_buffer);
void insere_fila_response_get_wait (char *buf, int indice, int tam_buffer);
void insere_fila_request_get (int indice);
void insere_fila_request_put(int indice, char *buf, int tam_buffer, unsigned long frame);
put_request *retira_fila_request_put ();
get_request *retira_fila_request_get();
get_response *retira_fila_response_get();
get_response *retira_fila_response_get_wait();
int tamanho_fila_request_get ();
int tamanho_fila_request_put ();
int arquivo_pode_utilizar (int indice, int arquivo_is_put);
void insere_lista_arquivos(int indice, int arquivo_is_put);
void remove_arquivo_lista (int indice);
int fila_request_put_vazia();
int fila_request_get_vazia();

SLIST_HEAD(, Arquivos) lista_arquivos;
STAILQ_HEAD(, GetResponse) fila_response_get_wait;
STAILQ_HEAD(, GetResponse) fila_response_get;
STAILQ_HEAD(, PutRequest) fila_request_put;
STAILQ_HEAD(, GetRequest) fila_request_get;

#endif
