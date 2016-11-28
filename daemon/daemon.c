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
    
FILE *fp;
    
#define MAXQUEUE 10   /* Numero maximo de conexoes que o servidor vai esperar */
#define BUFFERSIZE BUFSIZ
#define MAXCLIENTS 10
     
int main (int argc, char **argv)
{
  struct sockaddr_in servidor; /* Configuracao do socket principal */
  struct sockaddr_in cliente;  /* Configuracao do novo socket da conexao que do cliente */
  int sock;       /* Socket principal que faz o listen() */
  int sock_novo;  /* Socket de novas conexoes (cliente) */
  int porta;
  int size;
  int max_fd; /* Maximum file descriptor number */
  int num_bytes;
  fd_set master_fd; /* Lista principal de file descriptors */
  fd_set read_fds; /* Lista provisorio de file descriptors para o select */
  char buffer[BUFFERSIZE]; /* Buffer pra mensagem do cliente */
  socklen_t tam_endereco;
  
  /* Limpa os conjuntos de FDs */
  FD_ZERO(&master_fd);
  FD_ZERO(&read_fds);
  
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
  
  /* Adiciona o socket do servidor na lista principal */
  FD_SET(sock, &master_fd);
  
  /* Keep track of the biggest file descriptor */
  max_fd = sock;
  
  /* Loop principal do accept */
  while (1)
  {
    read_fds = master_fd;
    
    /* Select: numfds, readfds, writefds, exceptfds, timeVal */
    /* O que o select faz? */
    if (select(max_fd+1, &read_fds, NULL, NULL, NULL) == -1)
    {
      perror("select()");
      exit(1);
    }
    
    /* Run through the existing connections looking for date to read */
    int i;
    for (i = 0; i <= max_fd; i++)
    {
      /* Se existe algo para ler na lista */
      if (FD_ISSET(i, &read_fds))
      {
        if (i == sock)
        {
          /* Handle new connections */
          size = sizeof(struct sockaddr_in);
          sock_novo = accept(sock, (struct sockaddr *) &cliente, &size);
          if (sock_novo == -1)
          {
            perror("accept()");
            continue;
          }
          else
          {
            FD_SET(sock_novo, &master_fd); /* Adiciona na lista master */
            /* Keep track of the maximum */
            if (sock_novo > max_fd)
              max_fd = sock_novo;
            printf("selectserver: new connection from %s on socket %d\n", inet_ntoa(cliente.sin_addr), sock_novo);
          }
        }
        else
        {
          /* Handle data from a client */
          num_bytes = recv(i, buffer, sizeof(buffer), 0);
          if (num_bytes <= 0) /* Algum erro de recebimento */
          {
            if (num_bytes == 0)
            {
              printf("selectserver: socket %d hung up\n", i);
            }
            else
            {
              perror("recv()");
            }
            close(i);
            FD_CLR(i, &master_fd); /* Remove da lista master */
          }
          else /* Recebeu mensagem do cliente */
          {
            int j;
            for (j = 0; j < max_fd; j++)
            {
              /* Send to everyone */
              if (FD_ISSET(j, &master_fd))
              {
                /* Except the listener (sock) and ourselves */
                if (j != sock && j != i)
                {
                  if (send(j, buffer, num_bytes, 0) == -1)
                    perror("send()");
                }
              }
            }
          }
        }
      } 
    }
    
    /* Aceita conexao do cliente */
    
    
    /* Accpet bloqueia (sleep) ate receber alguma coisa */
  /*  sock_novo = accept(sock, (struct sockaddr *) &cliente, &size);
    if (sock_novo == -1)
    {
      perror("accept()");
      continue;
    }
    printf("Server: got connection from %s\n", inet_ntoa(cliente.sin_addr));*/
    
    /* Cria o processo child */
    /*pid = fork();
    if (pid > 0) /* Child process criado 
    {
      close(sock); /* Child doesn't need the listener 
      /* Socket do cliente manda mensagem 'Hello, world!' 
      if (send(sock_novo, "Hello, world!\n", 14, 0) == -1)
      {
        perror("send()");
      }
      close(sock_novo);
      exit(0);
    }
    else if (pid < 0) /* Erro na criacao do child process 
    {
      perror("fork()");
      exit(1);
    }
    close(sock_novo); /* Parent doesn't need this */
  }
  
  /* Aceita conexoes : cria o novo socket do cliente */
  /*int size = sizeof(struct sockaddr_in);
  sock_novo = accept(sock, (struct sockaddr *) &cliente, &size);
  if (sock_novo == -1)
  {
    perror("accpet()");
    exit(1);
  }*/
  
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

void formato_mesagem ()
{
  printf("Formato: ./recuperador <porta> <diretorio>\n");
}


