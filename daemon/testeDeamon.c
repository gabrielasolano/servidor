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

void formato_mesagem();
void inicia_servidor(int *sock, struct sockaddr_in *servidor, int porta);
void aceita_socket_cliente(int *sock_cliente, int sock_servidor, struct sockaddr_in *cliente, fd_set *master_fd, int *max_fd);
void configura_listas(fd_set *master_fd, fd_set *read_fds, int sock_servidor, int *max_fd);  
void responde_cliente (int socket_cliente);
/*FILE *fp;*/
    
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
  fd_set master_fd;
  fd_set read_fds;
  
  /* Valida quantidade de parametros de entrada */
  if (argc != 3)
  {
    printf("Parametros incorretos.\n");
    formato_mesagem();
    return 1;
  }
  
  /* Recupera porta do parametro de entrada */
  porta = atoi(argv[1]);
  
  /* Abre arquivo de log */
  /*fp = fopen("logDeamon.txt", "w+");*/
  
  /* socket > configura socket > bind > listen */
  inicia_servidor(&sock_servidor, &servidor, porta);
  
  /* Configuracao inicial das listas master e read */
  configura_listas(&master_fd, &read_fds, sock_servidor, &max_fd);
  
  /* Loop principal para aceitar e lidar com conexoes do cliente */
  while (1)
  {
    /* Adiciona as conexoes numa lista */
    read_fds = master_fd;
    if (select(max_fd+1, &read_fds, NULL, NULL, NULL) == -1)
    {
      perror("select()");
      exit(1);
    }
    
    /* Procura por requisicoes de leitura nas conexoes */
    for (i = 0; i <= max_fd; i++)
    {
      /* Existe requisicao de leitura */
      if (FD_ISSET(i, &read_fds))
      {
        /* Socket do servidor: aceita as conexoes novas */
        if (i == sock_servidor)
        {
          aceita_socket_cliente(&sock_cliente, sock_servidor, &cliente, &master_fd, &max_fd);
        }
        
        /* Socket dos clientes: lida com os dados dos cliente */
        else
        {
          responde_cliente(sock_cliente);
          FD_CLR(sock_cliente, &master_fd);
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

void configura_listas (fd_set *master_fd, fd_set *read_fds, int sock_servidor, int *max_fd)
{
  /* Zera as lista master e read (file descriptors) */
  FD_ZERO(&(*master_fd));
  FD_ZERO(&(*read_fds));
  
  /* Adiciona socket_servidor na lista master */
  FD_SET(sock_servidor, &(*master_fd));
  
  /* Controle dos sockets (file descriptors) */
  (*max_fd) = sock_servidor;  
}

void aceita_socket_cliente(int *sock_cliente, int sock_servidor, struct sockaddr_in *cliente, fd_set *master_fd, int *max_fd)
{
  socklen_t addrlen = sizeof(*cliente);
  (*sock_cliente) = accept(sock_servidor, (struct sockaddr *)&(*cliente), &addrlen);
  if ((*sock_cliente) == -1)
  {
    perror("accept()");
  }
  else
  {
    /* Adiciona o novo socket na lista master, atualiza o fd máximo */
    FD_SET((*sock_cliente), &(*master_fd));
    if ((*sock_cliente) > (*max_fd))
      (*max_fd) = (*sock_cliente);
    printf("Nova conexao de %s no socket %d\n", inet_ntoa((*cliente).sin_addr), (*sock_cliente));
  }
}

 /*!
 * Funcao para tratar o recebimento de mensagens no socket de um determinado
 * cliente n e enviar a resposta adequada.
 */
void responde_cliente (int sock_cliente)
{
  int bytes;
  char buffer[BUFFERSIZE];
  
  while ((bytes = recv((sock_cliente), buffer, sizeof(buffer), 0)) > 0)
  {
    printf("Entrou aqui!\nBuffer:\n%s", buffer);
    /*send(sock_cliente, "Ola", 4, 0);*/
  }
          
  if (bytes <= 0)
  {
    if (bytes == 0)
    {
      printf("Socket %d encerrou a conexao.\n", (sock_cliente));
    }
    else
    {
      perror("recv()");
    }
    close(sock_cliente);
  }
  
  /*struct stat st;
  char date[1024];
  char buf[BUFSIZE+1];
  char *aux;
  char *context;
  char *page;
  int tmp;
  int headerSize = 0;

  memset(buf, 0, sizeof(buf));

  if (clientsContext[n].active == 0)
  {
    clientsContext[n].active = 1;
    activeClients++;
  }

  if (clientsContext[n].msgReceived == 0)
  {
    if ((tmp = recv(clients[n], buf, sizeof(buf), 0)) > 0)
    {
      memcpy(clientsContext[n].msg + clientsContext[n].received, buf, tmp);
      clientsContext[n].received += tmp;
      memset(buf, 0, sizeof(buf));

      if(strstr(clientsContext[n].msg, "\r\n\r\n") != NULL)
      {
        clientsContext[n].msgReceived = 1;
      }
      else
      {
        return;
      }
    }

    if (tmp == 0)
    {
      fprintf(logfile, "Socket closed\n");
      closeClient(n);
      return;
    }
    else if (tmp < 0)
    {
      fprintf(logfile, "recv() error\n");
      closeClient(n);
      return;
    }
  }

  if (clientsContext[n].writting == 0)
  {
    fprintf(logfile, "Message received from socket %d:\n\n%s\n", clients[n],
          clientsContext[n].msg);
    aux = strtok_r(clientsContext[n].msg, " ", &context);

    if (strncmp(aux, "GET", 3) != 0)
    {
      aux = strtok_r(NULL, " ", &context);
      aux = strtok_r(NULL, "\r", &context);
      if ((strncmp(aux, "HTTP/1.0", 8) == 0) &&
          (strncmp(aux, "HTTP/1.1", 8) == 0))
      {
        char not_implemented[] = "HTTP/1.0 501 Not Implemented\r\n\r\n";
        send(clients[n], not_implemented, sizeof(not_implemented), 0);
        closeClient(n);
        return;
      }
      else
      {
        char bad_request[] = "HTTP/1.0 400 Bad Request\r\n\r\n";
        send(clients[n], bad_request, sizeof(bad_request), 0);
        closeClient(n);
        return;
      }
    }
    else
    {
      page = strtok_r(NULL, " ", &context);
      aux = strtok_r(NULL, "\r", &context);

      if ((strncmp(aux, "HTTP/1.0", 8) != 0) &&
          (strncmp(aux, "HTTP/1.1", 8) != 0))
      {
        char bad_request[] = "HTTP/1.0 400 Bad Request\r\n\r\n";
        send(clients[n], bad_request, sizeof(bad_request), 0);
        closeClient(n);
        return;
      }

      if (file_exist(page))
      {
        stat(page, &st);
        clientsContext[n].toSend = st.st_size;

        if ((clientsContext[n].fp = fopen(page,"rb")) == NULL)
        {
          fprintf(logfile, "fopen() error\n");
          char not_found[] = "HTTP/1.0 404 Not Found\r\n\r\n";
          send(clients[n], not_found, strlen(not_found), 0);
          closeClient(n);
          return;
        }

        memset(buf, '\0', sizeof(buf));

        time_t now = time(0);
        struct tm tm = *gmtime(&now);
        strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S %Z", &tm);
        sprintf(buf, "HTTP/1.0 200 OK\nDate: %s\nConnection: "
                     "Closed\r\n\r\n", date);

        headerSize = strlen(buf);


        clientsContext[n].read = fread(buf + headerSize, 1, (sizeof(buf) -
                                       headerSize), clientsContext[n].fp);

        tmp = send(clients[n], buf, (clientsContext[n].read + headerSize), MSG_NOSIGNAL);
        if (tmp <= 0)
        {
          fprintf(logfile, "send() error\n");
          closeClient(n);
          return;
        }

        clientsContext[n].writting = 1;

        clientsContext[n].sent += tmp;
        memset(buf, 0, sizeof(buf));

        fprintf(logfile, "%d bytes sent on socket %d\n", clientsContext[n].sent,
                clients[n]);

        if (clientsContext[n].sent >= clientsContext[n].toSend)
        {
          closeClient(n);
          return;
        }
      }
      else
      {
        fprintf(logfile, "File doesn't exist!\n\n");
        char not_found[] = "HTTP/1.0 404 Not Found\r\n\r\n";
        send(clients[n], not_found, strlen(not_found), 0);
        closeClient(n);
        return;
      }
    }
  }
  else
  {
    clientsContext[n].read = fread(buf, 1, sizeof(buf), clientsContext[n].fp);

    tmp = send(clients[n], buf, (clientsContext[n].read + headerSize), MSG_NOSIGNAL);

    if (tmp <= 0)
    {
      fprintf(logfile, "write() error\n");
      closeClient(n);
      return;
    }

    clientsContext[n].sent += tmp;
    memset(buf, 0, sizeof(buf));

    fprintf(logfile, "%d bytes sent on socket %d\n", clientsContext[n].sent,
                clients[n]);

    if (clientsContext[n].sent >= clientsContext[n].toSend)
    {
      closeClient(n);
      return;
    }
  }*/
 
}

void formato_mesagem ()
{
  printf("Formato: ./recuperador <porta> <diretorio>\n");
}


