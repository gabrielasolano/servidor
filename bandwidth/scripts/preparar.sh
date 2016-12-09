#!/bin/bash

cd ..
make
echo
echo

#OBS.: ./testeDeamon porta diretorio
echo "Criando pasta 'diretorio'"
mkdir -p diretorio
cd diretorio
echo
echo

#Recupera uma pagina HTML
echo "HTML"
echo
#wget http://lovelace.aker.com.br/buildbot-waterfall.html
echo
echo

#Recupera um aquivo BIN
echo "bin"
echo
mkdir Arquivos
cd Arquivos
#wget http://lovelace.aker.com.br/Arquivos/openwrt-ramips-mt7620-zbt-we826-squashfs-sysupgrade.bin
echo
echo

#Recupera um arquivo ISO
echo "ISO"
echo
cd ..
mkdir -p ISOs
cd ISOs
wget http://lovelace.aker.com.br/ISOs/CentOS-6.3-x86_64-minimal.iso
#wget http://lovelace.aker.com.br/ISOs/Fedora-18-i386-DVD.iso
echo
echo

#Recupera um arquivo PDF
echo "PDF"
echo
cd ../Arquivos
#wget http://lovelace.aker.com.br/Arquivos/ProjetoNovoDPIeIPS.pdf
echo "Arquivo PDF criado"
echo
echo

#Recupera um arquivo JPG
echo "JPG"
echo
cd ..
mkdir -p upload/tvcultura/programas
cd upload/tvcultura/programas
#wget http://tvcultura.com.br/upload/tvcultura/programas/programa-imagem-som.jpg
echo "Arquivo JPG criado"
echo
echo
echo "Script finalizado!"
echo "Para remover os arquivos criado execute o script limpar.sh"
