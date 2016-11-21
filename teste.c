#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

  /* argv[1]: URI completa 
   *      (protocolo (http://) + localizacao do recurso (site.com.br) + nome do recurso (/nome-do-recurso/))
   * argv[2]: nome do arquivo
   * Conectar ao servidor
   * Recuperar a p'agina
   * Salvar a p'agina no arquivo
   * 
   * Reportar erros:
   * 1. Linha de comando incompleta ou com parametros mal formados
   * 2. Diferentes erros de rede (servidor nao existe, nao responde, etc)
   * 3. Diferentes erros de arquivos (arquivo ja existe, nao pode ser acessado, etc)
   * a) Se arquivo ja existir, deve haver uma flag na linha de comando que force sua sobrescrita
   */

void parse_uri (char *argv, int porta, char *ip)
{
  /* http://192.168.0.1:8080/servlet/rece */
}
  
int main(int argc, char **argv)
{
  int socket_id;
  int conector;
  int i;
  int porta;
  struct sockaddr_in dest_addr;
  char *message, server_reply[2000], *ip;
  FILE *fp;
  
  /* Criar socket */
  socket_id = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_id < 0)
  {
    perror("Socket");
    return 1;
  }
  
  /* Parse da URI */
  parse_uri(argv[1], &porta, &ip);
  
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
    return 1;
  }
  puts("Reply received\n");
  puts(server_reply);
  
  /* Salva p'agina no arquivo */
  /* Percorre resposta do servidor at'e encontrar o in'icio do arquivo html */
  i = 0;
  while (server_reply[i] != '<')
  {
    i++;
  }
    
  /* Grava o html no arquivo informado */
  fp = fopen(argv[2], "w");
  while (server_reply[i] != '\0')
  {
    fprintf(fp, "%c", server_reply[i]);
    i++;
  }
  
  fclose(fp);
  return 0;
}
  
    
    
