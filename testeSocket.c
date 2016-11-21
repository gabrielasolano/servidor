#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h> /*inet_addr*/
#include <netinet/in.h>
#include <string.h>
 
int main(int argc , char *argv[])
{
  int socket_id;
  int conector;
  struct sockaddr_in dest_addr;
  char *message, server_reply[2000];
  FILE *fp;
  
  /* Criar socket */
  socket_id = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_id < 0)
  {
    perror("Socket");
    return 1;
  }
  
  /* Conecta com servidor remoto */
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(80);                           /* Porta HTTP do servidor */
  dest_addr.sin_addr.s_addr = inet_addr("216.58.213.35");   /* IP do servidor*/
  memset(&(dest_addr.sin_zero), '\0', 8);                   /* zera o resto da struct */
  
  conector = connect(socket_id, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));
  if (conector < 0)
  {
    perror("Conector");
    return 1;
  }

  /* Send some data */
  message = "GET / HTTP/1.0\r\n\r\n";
  if (send(socket_id, message, strlen(message), 0) < 0)
  {
    puts("Send failed.\n");
    return 1;
  }
  puts("Data send.\n");
  
  /* Recupera p'agina */
  if (recv(socket_id, server_reply, 2000, 0) < 0)
  {
    puts("recv failed.\n");
  }
  puts("Reply received\n");
  puts(server_reply);
  
  /* Salva p'agina no arquivo */
  /* Verifica se arquivo ja existe */
  if (fp = fopen(argv[2], "w"))
  {
    printf("Existe arquivo\n");
  }
  return 0;
}
