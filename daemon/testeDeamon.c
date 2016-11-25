/*! Conexao do servidor - single thread 
 *  
 * entrada: ./servidor porta diretório
 * 
 * O servidor recebe conexoes, logo ele tem que ser implementado com bind (porque vai 
 *  precisar do listen())
 * 
 * O cliente (primeira tarefa) requisita conexoes, entao pode utilizar apenas o connect
 * 
 * A porta de entrada deve ser uma acima de 1024 (abaixo desse numero 'e reservado)
 */

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void formato_mesagem();
void inicia_servidor(int *sock, struct sockaddr_in *servidor, int porta);
    
FILE *fp;
    
#define MAXQUEUE 10   /* Numero maximo de conexoes que o servidor vai esperar */
     
int main (int argc, char **argv)
{
  struct sockaddr_in servidor; /* Configuracao do socket principal */
  struct sockaddr_in cliente;  /* Configuracao do novo socket da conexao que do cliente */
  int sock;       /* Socket principal que faz o listen() */
  int sock_novo;  /* Socket de novas conexoes */
  int porta;
  
  /* Sao necessarios dois parametros na linha de comando 
   * ./servidor porta diretorio
   */
  if (argc != 3)
  {
    printf("Quantidade incorreta de parametros.\n");
    formato_mesagem();
    exit(1);
  }
  
  /* A porta 'e o primeiro parametro da linha de comando */
  porta = atoi(argv[1]);
  
  inicia_servidor(&sock, &servidor, porta);
  
  /* Aceita conexoes : cria o novo socket do cliente */
  int size = sizeof(struct sockaddr_in);
  sock_novo = accept(sock, (struct sockaddr *) &cliente, &size);
  if (sock_novo == -1)
  {
    perror("accpet()");
    exit(1);
  }
  
  close(sock);
  return 0;
}

void inicia_servidor(int *sock, struct sockaddr_in *servidor, int porta)
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
  
  /* Configura o socket: INADDR_ANY (bind recupera IP maquina que roda o processo */
  (*servidor).sin_family = AF_INET;
  (*servidor).sin_port = htons(porta); /*trocar 0 pela variavel porta*/
  (*servidor).sin_addr.s_addr = htonl(INADDR_ANY); 
  
  /* Conecta */
  if (bind((*sock), (struct sockaddr *) &(*servidor), sizeof(struct sockaddr_in)) == -1)
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

void formato_mesagem ()
{
  printf("Formato: ./recuperador www.pagina.com /path/arquivo T.\n");
  printf("T: flag optativa que forca a sobrescrita do arquivo.\n");
}


