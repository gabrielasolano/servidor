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
#include <fcntl.h>           /* Definition of AT_* constants */
#include <time.h>

void formato_mesagem();
void inicia_servidor(int *sock, struct sockaddr_in *servidor, int porta);
void responde_cliente (int socket_cliente, char diretorio[]);
int existe_pagina (char *caminho);
char *recupera_caminho (char *pagina, char diretorio[]);
char *recupera_mensagem (char *caminho);
    
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
  
  /* Muda o diretorio do processo para argv[2] */
  if (chdir(argv[2]) == -1)
  {
    perror("chdir()");
    exit(1);
  }
  
  /* Recupera diretorio absoluto */
  getcwd(diretorio, sizeof(diretorio));
  if (diretorio == NULL)
  {
    perror("getcwd()");
    exit(1);
  }
  
  printf("Iniciando servidor na porta: %d e no path: %s\n", porta, diretorio); 
  
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
          responde_cliente(sock_cliente, diretorio);
          FD_CLR(sock_cliente, &read_fds);
          //printf("Saiu aqui\n");
        }
      }
    }
  }
  close(sock_servidor);
  close(sock_cliente);
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
 * cliente e enviar a resposta adequada.
 */
void responde_cliente (int sock_cliente, char diretorio[])
{
  int bytes;
  int fecha_socket = 0;
  int size_strlen;
  char buffer[BUFFERSIZE+1];
  char *http_metodo;
  char *http_versao;
  char *pagina;
  char *contexto;
  char *mensagem;
  char *caminho;
  
  /* 1. Servidor recebe a mensagem do socket cliente
   * 2. Servidor responde a mensagem para o socket cliente
   * 3. Servidor fecha o socket cliente
   */
  //sleep(5);
  if ((bytes = recv(sock_cliente, buffer, sizeof(buffer), 0)) > 0)
  {
    printf("Recebeu mensagem: %s\n", buffer);
    
    /* Verifica se tem o GET no inicio */
    http_metodo = strtok_r(buffer, " ", &contexto);
    
    /* Se houve o GET no inicio :  */
    if (strncmp(http_metodo, "GET", 3) == 0)
    {
      /* Recupera a versao do protocolo HTTP : HTTP/1.0 ou HTTP/1.1 */
      pagina = strtok_r(NULL, " ", &contexto);
      http_versao = strtok_r(NULL, "\r", &contexto);
      
      /* Se nao houve a versao do protocolo HTTP : request invalido */
      if ((strncmp(http_versao, "HTTP/1.0", 8) != 0) && (strncmp(http_versao, "HTTP/1.1", 8) != 0))
      {
        mensagem = "HTTP/1.0 400 Bad Request\r\n\r\n";
      }
      
      /* Casos de request valido: GET /path/to/file HTTP/1.0 (ou HTTP/1.1) */
      else
      {
        /* Nao permite acesso fora do diretorio especificado */
        if (pagina[0] != '/')
        {
          mensagem = "HTTP/1.0 403 Forbidden\r\n\r\n";
        }
        
        else
        {
          caminho = recupera_caminho(pagina, diretorio);
          if (existe_pagina(caminho))
          {
            mensagem = recupera_mensagem(caminho);
          }
          else
          {
            mensagem = "HTTP/1.0 404 Not Found\r\n\r\n";
          } 
        }
      }
    }
    
    /* Se nao houver o GET no inicio : 501 Not Implemented */
    else
    {
      mensagem = "HTTP/1.0 501 Not Implemented\r\n\r\n";
    }
    
    size_strlen = strlen(mensagem);
    if (send(sock_cliente, mensagem, size_strlen, 0) == -1)
    {
      perror("send()");
    }
    fecha_socket = 1;
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

char *recupera_caminho (char *pagina, char diretorio[])
{
  int tam_pagina = strlen(pagina);
  int tam_diretorio = strlen(diretorio);
  char *destino;
  
  destino = (char *) malloc(tam_pagina + tam_diretorio);

  /* Recupera path absoluto da pagina */
  strncpy(destino, diretorio, tam_diretorio);
  strncat(destino, pagina, tam_pagina);
  
  return destino;
}

int existe_pagina (char *caminho)
{
  if (access(caminho, R_OK) == 0)
  {
    return 1; /* Arquivo existe com permissao para sua leitura */
  }
  return 0;
}

void formato_mesagem ()
{
  printf("Formato: ./recuperador <porta> <diretorio>\n");
  printf("Portas validas: 0 a 65535\n");
}

/* Recupera o arquivo que esta no path 'caminho' e grava em 'mensagem', junto com o cabecalho http */
char *recupera_mensagem (char *caminho)
{
  char *mensagem;
  char *buf;
  int tam_arquivo;
  int size_strlen;
  struct stat st;
  FILE *fp;
  
  /* Recupera o tamanho do arquivo */
  stat(caminho, &st);
  tam_arquivo = st.st_size;
  
  /* Constroi o cabecalho */
  size_strlen = strlen("HTTP/1.0 200 OK\r\n\r\n");
  
  mensagem = malloc(tam_arquivo + size_strlen);
  strncpy(mensagem, "HTTP/1.0 200 OK\r\n\r\n", size_strlen);
  
  /* Concatena o arquivo no cabecalho */
  fp = fopen(caminho, "rb");
  buf = malloc(sizeof(char *));
  
  while (tam_arquivo != 0)
  {
    fread(buf, 1, 1, fp);
    strncat(mensagem, buf, strlen(buf));
    tam_arquivo--;
  }
  
  /* Libera memoria */
  free(buf);
  fclose(fp);
  return mensagem;
}
