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

void formato_mesagem();
void inicia_servidor(int *sock, struct sockaddr_in *servidor, int porta);
void responde_cliente (int socket_cliente);
int existe_pagina (char *pagina);
    
#define MAXQUEUE 10   /* Numero maximo de conexoes que o servidor vai esperar */
#define BUFFERSIZE BUFSIZ
#define MAXCLIENTS 10
     
int main (int argc, char **argv)
{
  struct sockaddr_in servidor;
  struct sockaddr_in cliente;
  int sock_servidor;
  int sock_cliente;
  int porta;
  int max_fd;
  int i;
  fd_set read_fds;
  char diretorio[2000];
  socklen_t addrlen;
  
  /* Valida quantidade de parametros de entrada */
  if (argc != 3)
  {
    printf("Quantidade incorreta de parametros.\n");
    formato_mesagem();
    return 1;
  }
  
  /* Recupera e valida a porta */
  porta = atoi(argv[1]);
  if (porta < 0 || porta > 65535)
  {
    printf("Porta invalida!");
    formato_mesagem();
    return 1;
  }
  
  /*if (chdir(argv[2]) < 0)
  {
    perror("chdir()");
    exit(1);
  }

  getcwd(diretorio, sizeof(diretorio));
  printf("\nStarting Server on PORT: %s and PATH: %s\n", porta, diretorio);
  if (chroot(diretorio) < 0)
  {
    perror("chroot()");
    exit(1);
  }*/
  
  /* Recupera e valida o diretorio */
  /*strncpy(diretorio, argv[2], strlen(argv[2]));
  if (strlen(argv[2]) < strlen(diretorio))
  {
    diretorio[strlen(argv[2])] = '\0';
  }*/
  
  /* Muda o diretorio raiz do processo para o especificado em 'diretorio' */
  /*if (chroot(diretorio) == -1) 
  {
    perror("chroot()");
    exit(1);
  }*/
  
  /* socket > configura socket > bind > listen */
  inicia_servidor(&sock_servidor, &servidor, porta);

  /* Configuracao inicial da lista de sockets read_fds */
  FD_ZERO(&read_fds);
  FD_SET(sock_servidor, &read_fds);
  max_fd = sock_servidor;

  /* Loop principal para aceitar e lidar com conexoes do cliente */
  while (1)
  {
    /* Select retorna em read_fds os sockets que estao prontos para leitura */
    if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1)
    {
      perror("select()");
      exit(1);
    }
    
    /* Percorre sockets e verifica se estao na lista read_fds para serem tratados */
    for (i = 0; i <= max_fd; i++)
    {
      /* Existe requisicao de leitura */
      if (FD_ISSET(i, &read_fds))
      {
        /* Socket do servidor: aceita as conexoes novas */
        if (i == sock_servidor)
        {
          addrlen = sizeof(cliente);
          sock_cliente = accept(sock_servidor, (struct sockaddr *)&cliente, &addrlen);
          if (sock_cliente == -1)
          {
            perror("accept()");
          }
          else
          {
            FD_SET(sock_cliente, &read_fds);
            if (sock_cliente > max_fd)
              max_fd = sock_cliente;
            printf("Nova conexao de %s no socket %d\n", inet_ntoa(cliente.sin_addr), (sock_cliente));
          }
        }
        
        /* Socket dos clientes: lida com os dados dos cliente */
        else
        {
          //printf("Entrou aqui\n");
          responde_cliente(sock_cliente);
          FD_CLR(sock_cliente, &read_fds);
          //printf("Saiu aqui\n");
        }
      }
    }
  }
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
  (*servidor).sin_port = htons(porta);
  (*servidor).sin_addr.s_addr = htonl(INADDR_ANY); 
  memset(&((*servidor).sin_zero), '\0', 8); /* Zera o restante da struct */
  
  /* Conecta */
  if (bind((*sock), (struct sockaddr *) &(*servidor), sizeof(struct sockaddr)) == -1)
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

 /*!
 * Funcao para tratar o recebimento de mensagens no socket de um determinado
 * cliente n e enviar a resposta adequada.
 */
void responde_cliente (int sock_cliente)
{
  int bytes;
  int fecha_socket = 0;
  char buffer[BUFFERSIZE+1];
  char *http_metodo;
  char *http_versao;
  char *pagina;
  char *context;
  char *mensagem;
  char buf_salvo[BUFFERSIZE+1];
  
  /* 1. Servidor recebe a mensagem do socket cliente
   * 2. Servidor responde a mensagem para o socket cliente
   * 3. Servidor fecha o socket cliente
   */
  
  if ((bytes = recv(sock_cliente, buffer, sizeof(buffer), 0)) > 0)
  {
    printf("Recebeu mensagem: %s\n", buffer);
    
    //Salva o buffer, just in case (Se nao precisar, apagar)
    strncpy(buf_salvo, buffer, sizeof(buffer));
    
    /* Verifica se tem o GET no inicio */
    http_metodo = strtok_r(buffer, " ", &context);
    
    /* Se houve o GET no inicio :  */
    if (strncmp(http_metodo, "GET", 3) == 0)
    {
      /* Recupera a versao do protocolo HTTP : HTTP/1.0 ou HTTP/1.1 */
      pagina = strtok_r(NULL, " ", &context);
      http_versao = strtok_r(NULL, "\r", &context);
      
      /* Se nao houve a versao do protocolo HTTP : request invalido */
      if ((strncmp(http_versao, "HTTP/1.0", 8) != 0) && (strncmp(http_versao, "HTTP/1.1", 8) != 0))
      {
        mensagem = "HTTP/1.0 400 Bad Request\r\n\r\n";
      }
      
      /* Casos de request valido: GET /path/to/file HTTP/1.0 (ou HTTP/1.1) */
      else
      {
        printf("http_metodo: %s\npagina: %s\nhttp_versao: %s\n", http_metodo, pagina, http_versao);
        /* Verifica se a URI 'e valida */
        if (existe_pagina(pagina))
        {
          printf("Existe pagina!\n");
        }
        else
        {
          printf("Nao existe pagina!\n");
          mensagem = "HTTP/1.0 404 Not Found\r\n\r\n";
        }
      }
      send(sock_cliente, mensagem, strlen(mensagem), 0);
      fecha_socket = 1;
    }
    
    /* Se nao houver o GET no inicio : 501 Not Implemented */
    else
    {
      mensagem = "HTTP/1.0 501 Not Implemented\r\n\r\n";
      send(sock_cliente, mensagem, strlen(mensagem), 0);
      fecha_socket = 1;
    }
  }
  
  else if (bytes < 0)
  {
    perror("recv()");
  }
  else if (bytes == 0)
  {
    printf("Socket %d encerrou a conexao.\n", sock_cliente);
  }
  if (fecha_socket)
  {
    printf("Encerrada conexao com socket %d\n", sock_cliente);
    close(sock_cliente);
  }
}

int existe_pagina (char *pagina)
{
  /* Verifica se a URI existe : retorna 1 se existir, e 0 se nao existir */
  struct stat buffer;
  return (stat(pagina, &buffer) == 0);
}

void formato_mesagem ()
{
  printf("Formato: ./recuperador <porta> <diretorio>\n");
  printf("Portas validas: 0 a 65535\n");
}
