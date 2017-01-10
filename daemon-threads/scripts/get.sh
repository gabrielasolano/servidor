#!/bin/bash

for i in {1..10}; do
	#wget -c localhost:8080/../diretorio/CentOS-6.3-x86_64-minimal.iso -O resultado-get/imagem${i}.iso &
	#wget -c localhost:8080/../diretorio/buildbot-waterfall.html -O resultado-get/pagina${i}.html &
	#wget -c localhost:8080/../diretorio/planilha_bugs.ods -O resultado-get/planilha${i}.ods &
	wget -c localhost:8080/../diretorio/ProjetoNovoDPIeIPS.pdf -O resultado-get/projeto${i}.pdf &
done
