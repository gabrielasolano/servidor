#ifndef SERVIDOR_H
#define SERVIDOR_H

#include "funcoes_comuns.h"
#include "estruturas.h"
#include "threads.h"
#include "cliente.h"

void inicia_servidor (const int porta);
void encerra_servidor ();
void funcao_principal (const int porta);
void signal_handler (int signum);
void signal_interface (int signum);
void atualiza_readfd ();
int aceita_conexoes (int pedido_cliente);
int recebe_sinal_threads();
void processa_clientes(int recebido_from);
void atualiza_servidor();
void atualiza_porta (const int porta);
int recupera_configuracoes();
void atualiza_buffer_size();

#endif
