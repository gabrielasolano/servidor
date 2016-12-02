#!/bin/bash

echo
echo

#Recupera uma pagina HTML
./cliente localhost/buildbot-waterfall.html pagina.html
mv pagina.html diretorio
echo
echo

#Recupera um aquivo BIN
./cliente localhost/openwrt-ramips-mt7620-zbt-we826-squashfs-sysupgrade.bin arquivoBin.bin
mv arquivoBin.bin diretorio/Arquivos
echo
echo

#Recupera um arquivo ISO
./cliente localhost/CentOS-6.3-x86_64-minimal.iso arquivoIso.iso
mv arquivoIso.iso diretorio/ISOs
echo
echo

#Recupera um arquivo PDF
./cliente localhost/ProjetoNovoDPIeIPS.pdf arquivoPdf.pdf
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
