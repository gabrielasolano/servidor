#ifndef THREADS_H
#define THREADS_H

#include "funcoes_comuns.h"
#include "servidor.h"
#include "estruturas.h"

void cria_threads();
void *funcao_thread(void *id);

extern pthread_t threads[NUM_THREADS];

#endif
