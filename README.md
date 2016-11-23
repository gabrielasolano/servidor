# Treinamento Aker
## Recuperador de páginas WEB

Este programa deverá ser operado por linha de comando e compilado sobre Linux. Ele deverá receber como parâmetro uma URI completa e um nome de arquivo, e em seguida conectar-se ao servidor, recuperar a página e salvá-la no arquivo informado. Não é permitido usar libfetch ou outras bibliotecas equivalentes. Desta forma, o novo funcionário deve implementar o protocolo HTTP (versão 1.0) para obter os dados remotos. Informações a respeito do protocolo podem ser obtidas aqui.

Este programa deve se preocupar especialmente com as seguintes condições de erro e avisá-las ao usuário corretamente:

    Linha de comando incompleta ou com parâmetros mal formados;
    Diferentes erros de rede (servidor não existe, não responde, etc);
    Diferentes erros de arquivo (arquivo já existe, não pode ser acessado, etc).

Especificamente, no caso do arquivo informado já existir, deve haver uma flag na linha de comando que force sua sobrescrita.

Antes de concluída esta tarefa, a eficiência do recuperador deve ser avaliada e comparada com ferramentas de download equivalentes (por exemplo, wget).

O tempo esperado para o término desse exercício é de 20h de trabalho. Além do tratamento das condições de erro anteriores, é importante notar que as seguintes habilidades também serão avaliadas no exercício:

    Claridade na leitura do código fonte, o que inclui nomenclatura de variáveis, disposição dos elementos sintáticos, existência e qualidade de comentários, entre outros;
    Aderência completa às normas do protocolo tratado nesse exercício, o protocolo HTTP.

## Servidor WEB single-threaded

Este programa deverá ser um daemon para rodar em Linux. Ele deverá utilizar APIs padrão Berkeley Sockets, e deverá ser centrado em torno da função select ou poll. Este daemon não deverá criar novas threads ou processos para cada conexão, mas sim manter o estado de cada uma delas em uma lista de conexões ativas. O objetivo é familiarizar o novo programador com as técnicas de sockets não bloqueantes, e com outras necessárias ao desenvolvimento de daemons de rede de alta-performance.

Como parâmetros de linha de comando, o programa deve receber uma porta para escutar conexões e um diretório para ser tratado como raiz do servidor web.

Apenas os erros de existência de arquivo e falta de permissões de acesso devem ser reportados ao cliente. Deverá haver preocupações quanto às tentativas de recuperar arquivos fora do diretório especificado (usando-se coisas como ../../../../dado). Efetuar logs ou qualquer outra característica não essencial ao funcionamento mínimo do servidor é dispensável. Não é necessário o suporte a conexões persistentes (Keep-Alive).

Este servidor deve interoperar com o cliente desenvolvido anteriormente e com qualquer browser que se teste. As referências do protocolo devem ser sempre consultadas, de forma a manter o servidor aderente à norma.

Antes de concluída esta tarefa, a eficiência do servidor deve ser avaliada e comparada a servidores comerciais (por exemplo, Apache ou lighthttpd).

Note que o servidor deve lidar com as seguintes condições:

    Conexões feitas localmente com velocidades muito maiores do que o obtido normalmente;
    Várias conexões simultâneas (para simular leitura lenta, inserir temporariamente um delay de 200ms antes da chamada a read() ou fread() feita pelo servidor)

Um pequeno projeto deve ser elaborado para este servidor, de acordo com o procedimento relevante. Especial atenção deve ser tomada quanto ao diagrama de seqüência e os demais itens da especificação de requisitos. Todas as situações-limite e de erro devem ser documentadas e corretamente especificadas.

O trabalho deve ser concluído em 36h.
## Servidor WEB com controle de velocidade

Esse programa é uma evolução do anterior. Deve ser acrescentado o feature de controle de velocidade por conexão. Esse novo parâmetro de configuração determinará, em bytes/segundo, a taxa máxima de transferência de cada conexão efetuada por um cliente.

O trabalho deve ser concluído em 16h.
Servidor WEB com threads trabalhadoras

O servidor do exercício anterior deverá ser modificado para que use um pool de threads (de tamanho configurável em tempo de compilação) e para que suporte o comando PUT do protocolo HTTP. As threads serão usadas para ler (GET) e escrever (PUT) os dados dos arquivos pedidos pelos clientes. Desta forma, a thread principal não fica bloqueada em nenhum ponto (com exceção do select() ou poll()). Sempre que a thread principal quiser ler um arquivo pedido por um cliente, ela envia a solicitação em uma fila e uma das threads do pool deverá tratar esta leitura. Assim que a leitura for feita, a thread que leu avisa a thread principal, que irá tratar os dados lidos. A mesma idéia será usada para o PUT, isto é, a thread principal envia um pedido e alguma thread o trata. A comunicação entre a thread principal e as threads filhas deverá usar o mecanismo ‘condition’ e a comunicação entre as threads filhas e a thread principal deverá ser feita usando sockets locais (SOCK_DGRAM). A API Posix Threads (pthread) deverá ser usado neste exercício.

Durante a avaliação de performance, o número de threads deve ser variado e o efeito dessa variação determinada.

O trabalho deve ser concluído em 36h.
