#!/bin/bash

for i in {1..50}; do
	#curl -T ../diretorio/CentOS-6.3-x86_64-minimal.iso localhost:8080/scripts/resultado-put/imagem${i}.iso &
	#curl -T ../diretorio/buildbot-waterfall.html localhost:8080/scripts/resultado-put/pagina${i}.html &
	curl -T ../diretorio/planilha_bugs.ods localhost:8080/scripts/resultado-put/planilha${i}.ods &
	#curl -T ../diretorio/ProjetoNovoDPIeIPS.pdf localhost:8080/scripts/resultado-put/projeto${i}.pdf &
done
