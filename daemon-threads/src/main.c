#include "funcoes_comuns.h"
#include "servidor.h"

void formato_mensagem();

/*!
 * \brief Valida os parametros de entrada e chama a funcao principal do servidor
 * \param[in] argc Quantidade de parametros
 * \param[in] argv[1] Porta de conexao do servidor
 * \param[in] argv[2] Diretorio raiz
 * \param[in] argv[3] Banda maxima (opcional)
 */
int main (int argc, char **argv)
{
  int porta;
	char *home;

  if (argc < 3 || argc > 4)
  {
    printf("Quantidade incorreta de parametros.\n");
    formato_mensagem();
    return 1;
  }

  porta = atoi(argv[1]);
  if (porta < 0 || porta > 65535)
  {
    printf("Porta invalida!\n");
    formato_mensagem();
    return 1;
  }

	banda_maxima = 0;
	if (argc == 4)
	{
		banda_maxima = atol(argv[3]);
		if (banda_maxima <= 0)
		{
			printf("Taxa de envio invalida!\n");
			formato_mensagem();
			return 1;
		}
	}

	//daemon(1, 0);

  if (chdir(argv[2]) == -1)
  {
    perror("chdir()");
		exit(1);
  }

  getcwd(diretorio, sizeof(diretorio));
  if (diretorio == NULL)
  {
    perror("getcwd()");
		exit(1);
  }

	home = getenv("HOME");
	snprintf(config_path, sizeof(config_path), "%s/%s", home, CONFIG_PATH);
	snprintf(sock_path, sizeof(sock_path), "%s/%s", home, SOCK_PATH);
	snprintf(pid_path, sizeof(pid_path), "%s/%s", home, PID_PATH);

  escreve_arquivo_config(porta, diretorio, banda_maxima);

	escreve_arquivo_pid();

	funcao_principal(porta);

  return 0;
}

/*!
 * \brief Printa o formato de entrada necessario para execucao do programa
 */
void formato_mensagem ()
{
  printf("Formato 1: ./servidor <porta> <diretorio> <velocidade>\n");
	printf("Formato 2: ./servidor <porta> <diretorio>\n");
  printf("Portas validas: 0 a 65535\n");
	printf("Velocidades validas: maiores que 0\n");
}
