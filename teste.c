#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

  /* argv[1]: URI completa
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

int main(int argc, char **argv)
{
  int socket_desc;
  int connector;
  struct sockaddr_in server;
  
  /* Cria um socket */
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);  
  if ( socket_desc == -1 )
  {
    printf("Socket nao criado.\n");
  }
  
  server.sin_addr.s_addr = inet_addr("74.125.235.20");
  server.sin_family = AF_INET;
  server.sin_port = htons( 80 );
  
  /* Conecta em um servidor remoto (ip: google) */
  connector = connect(socket_desc , (struct sockaddr *)&server , sizeof(server));
  if ( connector < 0 )
  {
    printf("Erro na conecao.\n");
    return 1;
  }
  
  printf("Conectado\n");
  return 0;
}
  
    
    
