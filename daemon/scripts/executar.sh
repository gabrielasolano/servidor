#!/bin/bash
# Esse script 'e apenas para facilitar a visualizacao dos testes que eu fiz.
# Ele nao deve ser executado.

#Volta para a pasta 'deamon' e compila o servidor e o cliente
#cd ..
#make
#gcc cliente.c -o cliente
#./servidor 1096 diretorio &

#Baixa 10 arquivos de cada
for i in {1..10}; do
  wget localhost:8080/buildbot-waterfall.html -O pagina_${i}.html &
done

for i in {1..10}; do
	wget localhost:8080/Arquivos/openwrt-ramips-mt7620-zbt-we826-squashfs-sysupgrade.bin -O arquivoBin_${i}.bin &
done

for i in {1..10}; do
	wget localhost:8080/ISOs/CentOS-6.3-x86_64-minimal.iso -O arquivoIso_${i}.iso &
done

for i in {1..10}; do
	wget localhost:8080/Arquivos/ProjetoNovoDPIeIPS.pdf -O arquivoPdf_${i}.pdf &
done

for i in {1..10}; do
	wget localhost:8080/upload/tvcultura/programas/programa-imagem-som.jpg -O imagem_${i}.jpg &
done

echo "Script finalizado!"
echo "Para remover os arquivos criado execute o script limpar.sh"
