#ifndef CLIENTE_H
#define CLIENTE_H

#include "funcoes_comuns.h"
#include "estruturas.h"

void zera_struct_cliente (int indice);
void encerra_cliente (int indice);
void envia_cliente (int sock_cliente, char mensagem[], int size_strlen);
void cabecalho_parser (int indice);
void cabecalho_put (int indice);
int existe_diretorio (char *caminho);
void cabecalho_get (int indice);
void recebe_cabecalho_cliente (int indice);
int recupera_caminho (int indice, char *pagina);
int envia_cabecalho (int indice, char cabecalho[], int flag_erro);
int existe_pagina (char *caminho);
int recupera_tam_arquivo (int indice);
void recebe_arquivo_put (int indice);
int controle_banda (int indice);

int bad_request (int indice, char *http_versao, char *pagina);
#endif
