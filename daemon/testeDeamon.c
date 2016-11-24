/*Tarefa 2: Servidor Web single thread*/

#include <stdio.h>
#include <syslog.h>

void process ()
{
  /* Escreve no syslog */
  syslog(LOG_NOTICE, "Writing to my Syslog");
}

int main (int argc, char **argv)
{
  
  /* Permite escolher o que colocar (ou nao) no log */
  setlogmask(LOG_UPTO (LOG_NOTICE));
  
  /* Abri uma conexao com o syslog */
  openlog("testd", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
  
  process();
  
  /* Fecha a conecao com o syslog */
  closelog();
  
  return(0);
}
