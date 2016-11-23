#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <time.h> 
    
int criar_socket();
char *recupera_http(char *host, char *pagina);
void recupera_ip(char *host, char *ip);
void formato_mesagem();
void configura_socket(char *ip, struct sockaddr_in **remote);
void envia_http_servidor(int sock, char *http);
void recupera_pagina(int sock, char *pagina);
    
FILE *fp;
    
#define PORT 80
#define USERAGENT "HTMLGET 1.0"
     
int main(int argc, char **argv)
{
  struct sockaddr_in *remote;
  int sock;
  char *ip;
  char *http;
  char *host;
  char *pagina;
  char flag;
  
  /* Verifica quantidade de parametros da linha de comando */
  if ((argc != 3) && (argc != 4))
  {
    printf("Linha de comando incorreta!\n");
    formato_mesagem();
    exit(1);
  }
  else if (argc == 4)
  {
    flag  = argv[3][0];
  }
  
  host = argv[1];
  pagina = argv[2];
  
  /* Verificacao de arquivo */
  if ((fp = fopen(pagina, "rb")) != NULL)   /* Arquivo ja existe */
  {
    fclose(fp);
    if ((argc == 4) && (flag == 'T'))
    {
      fp = fopen(pagina, "wb");
    }
    else
    {
      perror("Arquivo ja existe");
      formato_mesagem();
      exit(1);
    }
  }

  sock = criar_socket();
  
  ip = malloc(16);       /* XXX.XXX.XXX.XXX\0 */
  recupera_ip(host, ip);
  printf("IP: %s\n", ip);
  
  configura_socket(ip, &remote);
  
  if (connect(sock, (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0)
  {
    perror("Erro ao conectar");
    exit(1);
  }
  
  http = recupera_http(host, pagina);
  printf("HTTP:\n%s\n", http);
  
  envia_http_servidor(sock, http);
  
  /* Recuperar pagina e grava no arquivo fp */
  recupera_pagina(sock, pagina);
  
  free(http);
  free(remote);
  free(ip);
  close(sock);
  return 0;
}

int criar_socket()
{
  int sock;
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
  {
    perror("Socket nao criado");
    exit(1);
  }
  return sock;
}

void recupera_ip (char *host, char *ip)
{
  struct hostent *hent;
  int tam_ip = 15;                            /* XXX.XXX.XXX.XXX */
  memset(ip, 0, tam_ip + 1);                  /* Seta string ip com 0s */

  if ((hent = gethostbyname(host)) == NULL)   /* Recupera IP a partir host */
  {
    perror("Nao foi possivel recuperar o IP");
    exit(1);
  }
  
  /* Converte endereco da rede em uma string (armazena em ip) */
  if (inet_ntop(AF_INET, (void *) hent->h_addr_list[0], ip, tam_ip) == NULL)
  {
    perror("Nao foi possivel converter host");
    exit(1);
  }
}

char *recupera_http (char *host, char *pagina)
{
  char *http;
  char *getpage = pagina;
  const char *cabecalho;
  int tam_host = strlen(host);
  int tam_getpage;
  int tam_useragent = strlen(USERAGENT);
  int tam_cabecalho;
  
  cabecalho = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
  tam_cabecalho = strlen(cabecalho);
  
  /* Remove '/' do inicio da string pagina (caso exista) */
  if(getpage[0] == '/'){
    getpage = getpage + 1;
  }
  tam_getpage = strlen(getpage);
  
  /* -5 considera os %s no cabecalho e o \0 final */
  http = (char *) malloc(tam_host + tam_getpage 
                          + tam_useragent + tam_cabecalho - 5);
  sprintf(http, cabecalho, getpage, host, USERAGENT);
  return http;
}

void configura_socket (char *ip, struct sockaddr_in **remote)
{
  int aux;

  (*remote) = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
  (*remote)->sin_family = AF_INET;
  (*remote)->sin_port = htons(PORT);
  aux = inet_pton(AF_INET, ip, (void *) (&((*remote)->sin_addr.s_addr)));
  
  if (aux < 0)                     
  {
    perror("Erro ao setar remote->sin_addr.s_addr");
    exit(1);
  }
  else if (aux == 0)              
  {
    perror("Endereco de IP informado 'e invalido");
    exit(1);
  }
}

void envia_http_servidor (int sock, char *http)
{
  int enviado = 0;
  int tamanho_http = strlen(http);
  int bytes;
  while (enviado < tamanho_http)
  {
    /* Retorna o numero de bytes enviado  */
    bytes = send(sock, http + enviado, tamanho_http - enviado, 0);   
    if(bytes == -1){
      perror("Erro ao enviar http");
      exit(1);
    }
    enviado += bytes;
  } 
}

/* Recuperar pagina e grava no arquivo fp */
void recupera_pagina (int sock, char *pagina)
{
  int htmlstart = 0;
  int estado = 0;
  int bytes;
  char buffer[1];
  double tempo_gasto;
  clock_t tempo_inicial, tempo_final;
  
  fp = fopen(pagina, "wb");
  
  /* Zera o buffer */
  memset(buffer, 0, sizeof(buffer));       
  
  /* Maquina de estados: ignora cabecalho inicial HTTP
   * e grava o restante no arquivo fp
   */
  tempo_inicial = clock();
  while ((bytes = recv(sock, buffer, sizeof(buffer), 0)) > 0)
  {
    if (htmlstart == 0)
    {
      if (estado == 0 && buffer[0] == '\r')
      {
        estado = 1;
      }
      else if (estado == 1 && buffer[0] == '\n')
      {
        estado = 2;
      }
      else if (estado == 2 && buffer[0] == '\r')
      {
        estado = 3;
      }
      else if (estado == 3 && buffer[0] == '\n')
      {
        htmlstart = 1;   
      }
      else
      {
        estado = 0;
      }
    }
    else if (htmlstart == 1)
    {
        fwrite(buffer, bytes, 1, fp);
    }
    memset(buffer, 0, bytes);  
  }
  
  if(bytes < 0)
  {
    perror("Erro no recebimento da mensagem");
  }
  tempo_final = clock();
  tempo_gasto = ((tempo_final - tempo_inicial) / CLOCKS_PER_SEC);
  printf("Tempo decorrido: %.0f segundos\n", tempo_gasto);
}

void formato_mesagem ()
{
  printf("Formato: ./recuperador www.pagina.com /path/arquivo T.\n");
  printf("T: flag optativa que forca a sobrescrita do arquivo.\n");
}
