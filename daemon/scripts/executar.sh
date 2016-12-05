#!/bin/bash

# Esse script 'e apenas para facilitar a visualizacao dos testes que eu fiz.
# Ele nao deve ser executado.

cd ..
make
# Em um terminal : ./servidor 1096 diretorio
echo
echo

#Recupera uma pagina HTML (em outro terminal)
./cliente localhost/buildbot-waterfall.html pagina.html
mv pagina.html diretorio
echo
echo

#Recupera um aquivo BIN (em outro terminal)
./cliente localhost/Arquivos/openwrt-ramips-mt7620-zbt-we826-squashfs-sysupgrade.bin arquivoBin.bin
mv arquivoBin.bin diretorio/Arquivos
echo
echo

#Recupera um arquivo ISO (em outro terminal)
./cliente localhost/ISOs/CentOS-6.3-x86_64-minimal.iso arquivoIso.iso
mv arquivoIso.iso diretorio/ISOs
echo
echo

#Recupera um arquivo PDF (em outro terminal)
./cliente localhost/Arquivos/ProjetoNovoDPIeIPS.pdf arquivoPdf.pdf
mv arquivoPdf.pdf diretorio/Arquivos
echo
echo

#Recupera um arquivo JPG
./cliente localhost/upload/tvcultura/programas/programa-imagem-som.jpg imagem.jpg
mv imagem.jpg diretorio/upload/tvcultura/programas
echo
echo

echo "Script finalizado!"
echo "Para remover os arquivos criado execute o script limpar.sh"
