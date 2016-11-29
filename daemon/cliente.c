/* Cliente */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define PORT 2014 /* Porta que o cliente vai conectar */
#define MAXDATASIZE 100 /* Max number of bytes er can get at once */

int main (int argc, char **argv)
{
  int sockfd, numbytes;
  char buf[MAXDATASIZE];
  struct hostent *he;
  struct sockaddr_in cliente;
  
  if (argc != 2)
  {
    fprintf(stderr, "usage: client hostname\n");
    exit(1);
  }
  
  if ((he = gethostbyname(argv[1])) == NULL)
  {
    herror("gethostbyname");
    exit(1);
  }
  
  if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
    exit(1);
  } 
  
  /* Configura o socket: INADDR_ANY (bind recupera IP maquina que roda o processo */
  cliente.sin_family = AF_INET;
  cliente.sin_port = htons(PORT);
  cliente.sin_addr = *((struct in_addr *)he->h_addr);
  memset(&(cliente.sin_zero), '\0', 8); /* Zera o restante da struct */
  
  if (connect(sockfd, (struct sockaddr *)&cliente, sizeof(struct sockaddr)) == -1)
  {
    perror("connect");
    exit(1);
  }
  char mensagem[] = " TESTE /path/to/file HTTP/1.0\r\n\r\n";
  send(sockfd, mensagem, strlen(mensagem), 0);
  if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1)
  {
    perror("recv");
    exit(1);
  }
  
  buf[numbytes] = '\0';
  printf("Received: %s\n", buf);
  close(sockfd);
  return 0;
}
