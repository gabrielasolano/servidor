#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
    
int criar_socket();
char *recupera_ip(char *host);
char *recupera_http(char *host, char *pagina);
    
FILE *fp;
    
#define PORT 80
#define USERAGENT "HTMLGET 1.0"
     
int main(int argc, char **argv)
{
  struct sockaddr_in *remote;
  int sock;
  int aux_remote;
  char *ip;
  char *http;
  char buffer[BUFSIZ+1];
  char *host;
  char *pagina;
  
  /* Verifica quantidade de parametros da linha de comando */
  if (argc != 3)
  {
    printf("Linha de comando incompleta.\n");
    return 1;
  }
  
  host = argv[1];
  pagina = argv[2];
  fp = fopen(pagina, "w");
  
  /* Verifica correta abertura do arquivo */
  if (fp == NULL)
  {
    perror("Erro ao abrir arquivo");
    exit(2);
  }
  
  /* Cria o Socket */
  sock = criar_socket();
  
  /* Recupera o IP */
  ip = recupera_ip(host);
  printf("IP: %s\n", ip);
  
  /* Seta endereco */
  remote = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
  remote->sin_family = AF_INET;
  remote->sin_port = htons(PORT);
  
  /* Converte a string ip em um endereco de rede (aloca em remote->sin_addr.s_addr) 
   * aux_remote = 1 : sucesso na conversao
   * aux_remote = 0 : ip nao contem um endereco valido para a family especificada
   * aux_remote = -1 : endereco family setado invalido
   */
  aux_remote = inet_pton(AF_INET, ip, (void *)(&(remote->sin_addr.s_addr)));
  if (aux_remote < 0)                     
  {
    perror("Erro ao setar remote->sin_addr.s_addr");
    exit(1);
  }
  else if (aux_remote == 0)              
  {
    perror("Endereco de IP informado 'e invalido");
    exit(1);
  }
  
  /* Conexao (com validacao de erro) */
  if (connect(sock, (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0)
  {
    perror("Erro ao conectar");
    exit(1);
  }
  
  /* Recupera a estruttura do HTTP */ 
  http = recupera_http(host, pagina);
  printf("HTTP:\n%s\n", http);
  
  /* Envia o HTTP para o servidor */
  int enviado = 0;
  while (enviado < strlen(http))
  {
    /* Retorna o numero de bytes enviado */
    aux_remote = send(sock, http+enviado, strlen(http)-enviado, 0);   
    if(aux_remote == -1){
      perror("Erro ao enviar http.\n");
      exit(1);
    }
    enviado += aux_remote;
  }
  
  /* Recuperar pagina e grava no arquivo fp */
  memset(buffer, 0, sizeof(buffer));            /* Zera o buffer */
  int htmlstart = 0;
  char * htmlcontent;
  while ((aux_remote = recv(sock, buffer, BUFSIZ, 0)) > 0) /* Retorna o tamanho da mensagem */
  {
    if (htmlstart == 0)                         /* Ignora o cabecalo HTTP*/
    {
      htmlcontent = strstr(buffer, "\r\n\r\n");
      if(htmlcontent != NULL)
      {
        htmlstart = 1;
        htmlcontent += 4;
      }
    }
    else                                        /* Recupera apenas o html */
    {
      htmlcontent = buffer;
    }
    if (htmlstart)                              /* Grava o html no arquivo */
    {
      fprintf(fp, "%s", htmlcontent);
    }
    
    memset(buffer, 0, aux_remote);  /* Zera o buffer para ao gravar o html diversas vezes */
  }
  
  if(aux_remote < 0)
  {
    perror("Error receiving data");
  }
  
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
    perror("Socket nao criado.\n");
    exit(1);
  }
  return sock;
}

char *recupera_ip(char *host)
{
  struct hostent *hent;
  int tamanho_ip = 15;                        /* XXX.XXX.XXX.XXX */
  char *ip = (char *) malloc(tamanho_ip + 1);
  memset(ip, 0, tamanho_ip + 1);              /* Seta string ip com 0s */
  
  if ((hent = gethostbyname(host)) == NULL)   /* Recupera IP a partir host */
  {
    herror("Nao foi possivel recuperar o IP");
    exit(1);
  }
  
  /* Converte endereco da rede em uma string (armazena em ip) */
  if (inet_ntop(AF_INET, (void *)hent->h_addr_list[0], ip, tamanho_ip) == NULL)
  {
    perror("Nao foi possivel converter host");
    exit(1);
  }
  return ip;
}

char *recupera_http(char *host, char *pagina)
{
  char *http;
  char *getpage = pagina;
  char *cabecalho = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
  
  if(getpage[0] == '/'){
    getpage = getpage + 1;
    printf("Removing leading \"/\", converting %s to %s\n", pagina, getpage);
  }
  /* -5 is to consider the %s %s %s in cabecalho and the ending \0 */
  http = (char *)malloc(strlen(host)+strlen(getpage)+strlen(USERAGENT)+strlen(cabecalho)-5);
  sprintf(http, cabecalho, getpage, host, USERAGENT);
  return http;
}
