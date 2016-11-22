# Treinamento Aker

1. Limpa código
2. Aceitar url com path (provavelmente alterar o recebimento do argv)
	>>nao eh possivel (gethostbyname não aceita)
	>>formato da entrada é sem http(s):// e sem paths no final
3. Não mais que 80 caracteres nas linhas (provável que precise quebrar)
4. strlen atribuído a variável
5. Preferível utilizar snprintf a sprintf
6. Recuperação/gravação de página com erro (é gravado string, tentar buffer streamming)
7. Trocar char *cabecalho por const char *cabecalho na função recupera_http
8. Criar makefile
9. Testar ./recuperador lovelace.aker.com.br/ISOs/CentOS-6.3-x86_64-minimal.iso /tmp/centos.iso
(verificar com wget wget http://lovelace.aker.com.br/ISOs/CentOS-6.3-x86_64-minimal.iso)
10. Usar máquina de estados na recuperação/gravação do arquivo (ao invés de utilizar um buffer size grande, utilizar o tamanho 1): ler um byte('\r') e vai para o estado 1, se ler o \n vai para o proximo estado, até ler o \r\n\r\n
11. Não pode criar o arquivo se der erro na abertura


OBS.: Não acho que assim vá funcionar, já que a ISO não está em lovelace.aker.com.br e sim em lovelacer.aker.com.br/ISOs/

mkdir -p ISOs
./recuperador lovelace.aker.com.br ISOs/CentOS-6.3-x86_64-minimal.iso T
cd ISOs
wget http://lovelace.aker.com.br/ISOs/CentOS-6.3-x86_64-minimal.iso
ls -lh
